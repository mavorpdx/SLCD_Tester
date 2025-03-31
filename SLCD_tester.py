import argparse
import json
import os
import serial
import time
import logging
import re
from colorama import init, Fore, Style, Back as bg
init(autoreset=True)
import signal
import sys
from datetime import datetime, timedelta
import string
import traceback

'''
Usage

Configure Device
$ python g1-plus-tester.py -dc dev_config.json


Get Documentation
$ python g1-plus-tester.py -d -v3 main.json

Run Tests
$ python g1-plus-tester.py -v1 main.json

'''

# **********************************************************************************************
#  Global Variables
# **********************************************************************************************
codeVersion = '3.0.1' 
serial_port = 'COM3'
baudrate = 115200
totalErrors = 1
carriage_return = '\r'
actionList = ["simple", "complex", "binary", "binOut", "quiet", "device_specific", "regex", "instruction", "command"]
sleep0 = 0.01
sleep1 = 0.1
sleep5 = 0.7


packet_processing_enabled = True  # Enable packet processing
pending_packet = ""  # Buffer for accumulating partial packets
debug_packets = []  # Store debug packets (square brackets)
command_results = []  # Store command results (curly brackets)


CRLF = "\n\r"

translation_table = str.maketrans('', '', string.whitespace)

TEST_NAME_WIDTH = 50  # Setting for the column width of the test name.
TEST_COLUMN_WIDTH = TEST_NAME_WIDTH + 22
global verbose 
verbose = 0
errCount = 0
passCount = 0

Level_DEBUG = True
Level_INFO = True
Level_WARNING = True
Level_ERROR = True
Level_CRITICAL = True
Level_COMMENT = True

exit_program = False
exit_current_test = False

read_two_lines = False 

fileCnt = 0   # used to track the number of JSON files added to the list

# **********************************************************************************************
#  Device Configuration Functions
# **********************************************************************************************
def load_device_config(config_file):
    """
    Load a device configuration JSON file.
    
    Input:
    - config_file (str): Path to the device configuration file
    
    Output:
    - dict or None: Device configuration data if successful, None otherwise
    """

    print_comment(f"**************************************************")
    print_comment(f"Loading device configuration file: {config_file}")
    
    if not config_file.endswith('.json'):
        config_file += '.json'
    
    try:
        with open(config_file, 'r') as f:
            device_config = json.load(f)
            print_debug(f"Device config file {config_file} loaded successfully")
            
            # Print basic configuration info
            if "device_info" in device_config:
                name = device_config["device_info"].get("name", "Not specified")
                part_number = device_config["device_info"].get("part_number", "Not specified")
                display_type = device_config["device_info"].get("display_type", "Not specified")
                print_comment(f"Device name: {name}")
                print_comment(f"Part number: {part_number}")
                print_comment(f"Display type: {display_type}")
            
            return device_config
    except FileNotFoundError:
        print_error(f"Device config file {config_file} not found.")
        return None
    except json.JSONDecodeError:
        print_error(f"Error parsing {config_file}. Invalid JSON format.")
        return None
    except Exception as e:
        print_error(f"Error loading device configuration: {str(e)}")
        return None

def apply_device_config(ser, device_config):
    """
    Apply device configuration via serial port.
    
    Input:
    - ser (serial.Serial): Serial port object
    - device_config (dict): Device configuration data
    
    Output:
    - bool: True if configuration was successfully applied, False otherwise
    """
    if not ser or not device_config:
        print_error("Cannot apply device configuration: Serial port not open or configuration not loaded.")
        return False
    
    try:
        print_debug("Applying device configuration...")
        
        # Process all sections except device_info
        for section_name, section_data in device_config.items():

            # Skip device_info section
            if section_name == "device_info":
                continue
            
            print_info(f"Configuring {section_name}...")
            
            for key, value in section_data.items():
                #print_debug(f"Processing key: {key}, value: {value}, type: {type(value)}")
                if isinstance(value, bool):
                    if value:
                        command = f"set {key}"
                    else:
                        command = f"clr {key}"
                    #print_debug(f"Sending command '{command}'")
                    send_serial_string(ser, command)
                    read_serial_with_packet_processing(ser, 1.0)

                else:
                    # Handle other types of settings
                    command = f"{key} {value}"
                    #print_debug(f"Setting {key}: sending command '{command}'")
                    send_serial_string(ser, command)
                    read_serial_with_packet_processing(ser, 1.0)

        print_debug("Device configuration applied successfully!")
        return True
    
    except Exception as e:
        print_error(f"Error applying device configuration: {str(e)}")
        print_error(traceback.format_exc())
        return False

def process_device_config(config_file):
    """
    Process a device configuration file and apply settings.
    
    Input:
    - config_file (str): Path to the device configuration file
    
    Output:
    - bool: True if configuration was successfully processed and applied, False otherwise
    """
    # Load the device configuration
    device_config = load_device_config(config_file)
    if not device_config:
        return False
    
    # Open the serial port
    ser = open_serial_port(config)
    if not ser:
        print_error("Failed to open serial port. Cannot apply device configuration.")
        return False
    
    try:
        # Apply the device configuration
        success = apply_device_config(ser, device_config)
        
        # Save settings to device if needed
        if success and args.save_config:
            print_debug("Saving configuration to device memory...")
            send_serial_string(ser, "SAVE")
            time.sleep(1)  # Wait for save operation to complete
            print_debug("Configuration saved to device memory.")
        
        return success
    
    finally:
        send_serial_string(ser, "VERS")
        read_serial_with_packet_processing(ser, 1.0)
        close_serial_port(ser)


def extract_response_value(received_data):
    """Extract the actual response value from command packets."""
    print_debug(f"DEBUG EXTRACT - Raw received: '{received_data}'")
    
    # Look for command packets with a value
    command_matches = re.findall(r'\{"COM\d+":\s*"([^"]*)"\}', received_data)

    print_debug(f"DEBUG EXTRACT - Matches found: {command_matches}")
    
    # If we found any command packets with non-empty values, return the first one
    for value in command_matches:
        if value and value != "":
            print_debug(f"DEBUG EXTRACT - Returning extracted value: '{value}'")
            return value
    
    # If no valid command packet value was found, return the original string
    print_debug(f"DEBUG EXTRACT - No match found, returning original: '{received_data}'")
    return received_data


# **********************************************************************************************
#  Packet Processing Functions
# **********************************************************************************************
def process_packet(packet):
    """Process a packet from the serial port."""
    
    #print_debug(f"Received packet: '{packet}', length: {len(packet)}")

    if not packet or len(packet) < 2:
        return None, None
        
    if packet[0] == '[' and packet[-1] == ']':
        debug_message = packet[1:-1].strip()
        print_debug(f"DEBUG '{debug_message}'")
        
    elif packet[0] == '{' and packet[-1] == '}':
        # Command packet processing
        content = packet[1:-1].strip()
        
        if " : " in content:
            parts = content.split(" : ", 1)
            if len(parts) == 2:
                port, result = parts[0].strip('"'), parts[1].strip('"')
                #print_debug(f"DEBUG Parsed - Port: '{port}', Result: '{result}'")
                command_results.append((port, result))
                return False, result
        
        return False, content
            
    return None, None

def read_serial_with_packet_processing(ser, timeout=1.0):
    global pending_packet
    
    start_time = time.time()
    cursor_value = None
    packet_results = []  # Store all processed packets

    while time.time() - start_time < timeout:
        if ser.in_waiting > 0:
            chunk = ser.read(ser.in_waiting).decode('ascii', errors='replace')
            #print_debug(f"DEBUG READ - Raw received chunk: '{chunk}'")
            pending_packet += chunk
            
            # Check for cursor characters anywhere in the chunk
            if '>' in chunk:
                cursor_value = '>'
                print_debug(f"DEBUG Found cursor: '{cursor_value}'")
            elif '!' in chunk:
                cursor_value = '!'
                print_debug(f"DEBUG Found cursor: '{cursor_value}'")
            
            # Process all packets before returning
            processed_all = False
            while not processed_all:
                # Check for square bracket packets
                square_bracket_packets = re.findall(r'\[.*?\]', pending_packet)
                if square_bracket_packets:
                    print_debug(f"DEBUG Found square brackets: {square_bracket_packets}")
                    
                    # Process each square bracket packet
                    for packet in square_bracket_packets:
                        # Process the packet
                        is_debug, result = process_packet(packet)
                        # Remove the processed packet from pending_packet
                        pending_packet = pending_packet.replace(packet, '', 1)
                        
                        if is_debug is not None:
                            packet_results.append(('debug', result))
                    
                    continue  # Check for more packets after processing
                
                # Check for curly bracket packets
                curly_bracket_packets = re.findall(r'\{.*?\}', pending_packet)
                if curly_bracket_packets:
                    for packet in curly_bracket_packets:
                        print_debug(f"RSP: {packet}")
                        is_debug, result = process_packet(packet)
                        if is_debug is not None and not is_debug:
                            # Remove the processed packet from pending_packet
                            pending_packet = pending_packet.replace(packet, '', 1)
                            packet_results.append(('command', result))
                    continue  # Check for more packets after processing
                
                # Clean up any remaining whitespace or control characters
                if pending_packet.strip() == "":
                    pending_packet = ""  # If only whitespace remains, clear it entirely
                
                # Clean up any CR/LF characters
                pending_packet = re.sub(r'^[\r\n\s]+', '', pending_packet)  # Remove leading whitespace/CR/LF

                # If we get here, no more packets were processed
                processed_all = True
        
        # If we have any results, return the last processed packet
        if packet_results:
            # Return the last command packet if available, otherwise the last debug packet
            command_results = [r for r in packet_results if r[0] == 'command']
            if command_results:
                return 'command', command_results[-1][1], cursor_value
            return packet_results[-1][0], packet_results[-1][1], cursor_value
        
        time.sleep(sleep1)
    
    # Check for timeout with incomplete packet
    if pending_packet:
        print_debug(f"Incomplete packet after timeout: {pending_packet}")
    
    # If no packets were processed but we received data
    if not packet_results and pending_packet:
        return None, pending_packet, cursor_value
    
    return None, None, cursor_value

def reset_packet_buffers():
    """Reset all packet processing buffers."""
    global pending_packet, debug_packets, command_results
    pending_packet = ""
    debug_packets = []
    command_results = []
    print_debug("Packet buffers reset")


# **********************************************************************************************
#  Logging 
# **********************************************************************************************
global log_file_name
global color_enabled
color_enabled = True  # Default to color enabled

dispCurs = False
dispStr = True
dispBytes = False
dispHex = False

global logging_enabled
logging_enabled = False

# **********************************************************************************************
#  Read Master Version
#  Input:
#  - master_file (str): Path to the master file
#
#  Output:
#  - str: Version string from the master file, or 'Unknown' if not found or error occurs
# **********************************************************************************************
def read_master_version(master_file):
    try:
        with open(master_file, 'r') as f:
            master_data = json.load(f)
        return master_data.get('version', 'Unknown')
    except Exception as e:
        print_error(f"Error reading master file version: {str(e)}")
        return 'Unknown'

# **********************************************************************************************
#  Change Verbosity
#  Input:
#  - newVerbose (int): New verbosity level (0-3)
#
#  Output:
#  - None (modifies global variables)
# **********************************************************************************************
def changeVerbosity(newVerbose):
    global verbose, Level_DEBUG, Level_INFO, Level_WARNING, Level_ERROR, Level_CRITICAL, Level_COMMENT, color_enabled
    
    try:
        verbose = newVerbose  # Set the global verbose variable

        if (newVerbose == 0) :
            Level_INFO = True
            Level_COMMENT = False
            Level_WARNING = False
            Level_DEBUG = False
            Level_ERROR = False
            Level_CRITICAL = False

        elif (newVerbose == 1) :
            Level_INFO = True
            Level_COMMENT = True
            Level_WARNING = False
            Level_DEBUG = False
            Level_ERROR = False
            Level_CRITICAL = False

        elif (newVerbose == 2) :
            Level_INFO = True
            Level_COMMENT = True
            Level_WARNING = True
            Level_DEBUG = False
            Level_ERROR = False
            Level_CRITICAL = False

        elif (newVerbose == 3) :
            Level_INFO = True
            Level_COMMENT = True
            Level_WARNING = True
            Level_DEBUG = True
            Level_ERROR = True
            Level_CRITICAL = True

        #if verbose > 2:  # Only print debug messages if verbosity is 3 or higher
        print_debug("VERBOSE = " + str(newVerbose))
        print_debug("DEBUG = " + str(Level_DEBUG))
        print_debug("INFO = " + str(Level_INFO))
        print_debug("WARNING = " + str(Level_WARNING))
        print_debug("ERROR = " + str(Level_ERROR))
        print_debug("CRITICAL = " + str(Level_CRITICAL))
        print_debug("COMMENT = " + str(Level_COMMENT))

    except Exception as e:
        print_error(f"Error in changeVerbosity: {str(e)}")

# **********************************************************************************************
#  Print Info Message
#  Input:
#  - message (str): The message to print
#
#  Output:
#  - None (prints to console and/or log file)
# **********************************************************************************************
def print_info(message):
    if Level_INFO:
        formatted_message = f"{Fore.WHITE}{message}{Fore.RESET}{bg.RESET}" if color_enabled else message
        print_always(formatted_message)
        if logging_enabled:
            logging.info(strip_color_codes(message))

# **********************************************************************************************
#  Print Debug Message
#  Input:
#  - message (str): The message to print
#
#  Output:
#  - None (prints to console and/or log file)
# **********************************************************************************************
def print_debug(message):
    if Level_DEBUG:
        formatted_message = f"{Fore.BLUE}{Style.BRIGHT}{message}{Fore.RESET}{bg.RESET}" if color_enabled else message
        print_always(formatted_message)
        if logging_enabled:
            logging.debug(strip_color_codes(message))

def print_warning(message):
    if Level_WARNING:
        formatted_message = f"{Fore.CYAN}{message}{Fore.RESET}{bg.RESET}" if color_enabled else message
        print_always(formatted_message)
        if logging_enabled:
            logging.warning(strip_color_codes(message))

def print_error(message):
    if Level_ERROR:
        formatted_message = f"{bg.WHITE}{Fore.RED}{message}{Fore.RESET}{bg.RESET}" if color_enabled else message
        print_always(formatted_message)
        if logging_enabled:
            logging.error(strip_color_codes(message))

def print_critical(message):
    if Level_CRITICAL:
        formatted_message = f"{bg.WHITE}{Fore.RED}{Style.BRIGHT}{message}{Fore.RESET}{bg.RESET}" if color_enabled else message
        print_always(formatted_message)
        if logging_enabled:
            logging.critical(strip_color_codes(message))

def print_comment(message):
    if Level_COMMENT:
        formatted_message = f"{Fore.GREEN}{message}{Fore.RESET}{bg.RESET}" if color_enabled else message
        print_always(formatted_message)
        if logging_enabled:
            logging.info(strip_color_codes(message))

# **********************************************************************************************
#  Print Always
#  Input:
#  - message (str or bytes): The message to print
#  - end (str): The string appended after the message (default: '\n')
#
#  Output:
#  - None (prints to console and/or log file)
# **********************************************************************************************
def print_always(message, end='\n'):
    global color_enabled, logging_enabled
    if isinstance(message, bytes):
        message = message.hex()

    if color_enabled:
        print(f"{Fore.WHITE}{message}{Fore.RESET}{bg.RESET}", end=end)
    else:
        print(message, end=end)

    if logging_enabled and not message.startswith('["') and not message.startswith('{"'):
        logging.info(strip_color_codes(message))


# **********************************************************************************************
#  Strip Color Codes
#  Input:
#  - text (str): Text containing ANSI color codes
#
#  Output:
#  - str: Text with color codes removed
# **********************************************************************************************
def strip_color_codes(text):
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    return ansi_escape.sub('', text)

# **********************************************************************************************
#  Setup Logging
#  Input:
#  - log_enabled (bool): Whether logging is enabled
#  - log_file (str): Name of the log file
#  - verbose (int): Verbosity level
#  - serial_number (str, optional): Serial number to include in the log file name
#
#  Output:
#  - str or None: Name of the log file if logging is enabled, None otherwise
# **********************************************************************************************
def setup_logging(log_enabled, log_file, verbose, serial_number=None, log_with_details=False):
    global logging_enabled
    log_file_name = None

    try:
        if log_enabled:
            logging_enabled = True
            log_level = logging.INFO if verbose == 0 else logging.DEBUG
            
            class CustomFormatter(logging.Formatter):
                def format(self, record):
                    return record.getMessage()

            # Create the log file name with serial number and date/time
            current_time = datetime.now().strftime("%y%m%d_%H%M")
            sn = serial_number if serial_number else "NoSerialNumber"
            
            if log_with_details:
                # Create a temporary file name without PASS/FAIL
                log_file_name = f"{sn}_{current_time}.log"
            else:
                log_file_name = log_file

            # Set up file logging
            file_handler = logging.FileHandler(log_file_name, mode='w')  
            file_handler.setLevel(log_level)
            file_handler.setFormatter(CustomFormatter())

            # Configure root logger
            logger = logging.getLogger('')
            logger.setLevel(log_level)
            logger.addHandler(file_handler)

            # Remove any existing handlers (including the default StreamHandler)
            for handler in logger.handlers[:]:
                if isinstance(handler, logging.StreamHandler) and handler != file_handler:
                    logger.removeHandler(handler)

        else:
            logging_enabled = False
            logging.disable(logging.CRITICAL)
    except Exception as e:
        print_debug(f"Error setting up logging: {str(e)}")
        logging_enabled = False

    return log_file_name

# **********************************************************************************************
#  Signal Handler
#  Input:
#  - signum (int): Signal number
#  - frame: Current stack frame
#
#  Output:
#  - None (sets global variables and may raise SystemExit)
# **********************************************************************************************
def signal_handler(signum, frame):
    global exit_program, exit_current_test
    if signum == signal.SIGINT:
        print_info("\nControl-C detected. Exiting program...")
        exit_program = True
        raise SystemExit
    elif signum in (signal.SIGBREAK, signal.SIGUSR1):
        print_info("\nControl-Break detected. Exiting current test...")
        exit_current_test = True

# Set up signal handlers
signal.signal(signal.SIGINT, signal_handler)
if sys.platform == "win32":
    signal.signal(signal.SIGBREAK, signal_handler)
else:
    signal.signal(signal.SIGUSR1, signal_handler)

# **********************************************************************************************
#  Load Config
#  Input:
#  - config_file (str): Path to the configuration file (default: 'config.json')
#
#  Output:
#  - dict: Configuration data
# **********************************************************************************************
def load_config(config_file='config.json'):

    print_debug(f"**************************************************")
    print_debug(f"Loading config file: {config_file}")
    if not config_file.endswith('.json'):
        config_file += '.json'
    try:
        with open(config_file, 'r') as f:
            config = json.load(f)
            print_debug(f"Config file {config_file} loaded successfully")
            print_debug(f"Using serial port: {config.get('serial_port', 'Not specified')}")
            print_debug(f"Using baudrate: {config.get('baudrate', 'Not specified')}")
        return config
    except FileNotFoundError:
        print_error(f"Config file {config_file} not found. Using default settings.")
        default_config = {"serial_port": "/dev/ttyACM0", "baudrate": 115200}
        print_error(f"Using default serial port: {default_config['serial_port']}")
        print_error(f"Using default baudrate: {default_config['baudrate']}")
        return default_config
    except json.JSONDecodeError:
        print_error(f"Error parsing {config_file}. Using default settings.")
        default_config = {"serial_port": "COM3", "baudrate": 115200}
        print_error(f"Using default serial port: {default_config['serial_port']}")
        print_error(f"Using default baudrate: {default_config['baudrate']}")
        return default_config

# **********************************************************************************************
#  Load Identity
#  Input:
#  - identity_file (str): Path to the identity file (default: 'identity.json')
#
#  Output:
#  - dict: Identity data
# **********************************************************************************************

def load_identity(identity_file='identity.json'):
    if not identity_file.endswith('.json'):
        identity_file += '.json'
    try:
        with open(identity_file, 'r') as f:
            identity = json.load(f)
        return identity
    except FileNotFoundError:
        print_comment(f"Identity file {identity_file} not found.  Trying default file.")
        with open('identity.json', 'r') as f:
            identity = json.load(f)
        return identity
    except json.JSONDecodeError:
        print_error(f"Error parsing {identity_file}. Using default settings.")
        return {}

# **********************************************************************************************
#  Get Config Value
#  Input:
#  - config (dict): Configuration dictionary
#  - name (str): Name of the configuration value to retrieve
#
#  Output:
#  - Any: The value associated with the given name, or None if not found
# **********************************************************************************************
def get_config_value(config, name):

    print_debug(f"Device-specific lookup for: {name}")
    original_value = None
    alt_value = None
    
    device_specific = config.get('deviceSpecific', [])
    
    if not isinstance(device_specific, list):
        print_debug(f"Warning: 'deviceSpecific' is not a list in the config. Type: {type(device_specific)}")
        return original_value, alt_value

    for pair in device_specific:
        if not isinstance(pair, dict):
            print_debug(f"Warning: Unexpected item in 'deviceSpecific'. Expected dict, got {type(pair)}")
            continue

        pair_name = pair.get('name')
        pair_value = pair.get('value')

        if pair_name == name:
            original_value = pair_value
            print_debug(f"Config value found for {pair_name}: {original_value}")
        elif pair_name == f"{name}_alt":
            alt_value = pair_value
            print_debug(f"Alternate config value found for {pair_name}: {alt_value}")
    
    if original_value is None and alt_value is None:
        print_debug(f"No config value found for {name} or {name}_alt")
    
    return original_value, alt_value

# **********************************************************************************************
#  Print Identity Data
#  Input:
#  - identity (dict): Identity data dictionary
#
#  Output:
#  - None (prints to console)
# **********************************************************************************************
def print_identity_data(identity):
    device_info = [
        "**************************************************",
        f"deviceFamily: {identity.get('deviceFamily', 'Unknown')}",
        f"panelType: {identity.get('panelType', 'Unknown')}",
        f"deviceNumber: {identity.get('deviceNumber', '')}",
        f"assemblyNumber: {identity.get('assemblyNumber', 'Unknown')}",
        "**************************************************",
        f"SLCD+ Tester ver: {codeVersion}"
    ]
    
    for line in device_info:
        print_debug(line)



# **********************************************************************************************
#  Open Serial Port
#  Input:
#  - config (dict): Configuration dictionary containing 'serial_port' and 'baudrate'
#
#  Output:
#  - serial.Serial or None: Open serial port object if successful, None otherwise
# **********************************************************************************************
def open_serial_port(config):
    try:
        ser = serial.Serial(config['serial_port'], config['baudrate'], timeout=5, inter_byte_timeout=1)
        if packet_processing_enabled:
            reset_packet_buffers()
            print_debug("Packet processing initialized for new serial port connection")
        return ser
    except serial.SerialException as e:
        print_error(f"Failed to open serial port: {e}")
        return None

# **********************************************************************************************
#  Close Serial Port
#  Input:
#  - ser (serial.Serial): Serial port object to close
#
#  Output:
#  - None
# **********************************************************************************************
def close_serial_port(ser):
    if ser is not None:
        ser.close()
        if (Level_DEBUG) :
            print_debug("Serial port closed")

# **********************************************************************************************
#  Send Serial String
#  Input:
#  - ser (serial.Serial): Serial port object
#  - data (str): Data to send
#
#  Output:
#  - None
# **********************************************************************************************
def send_serial_string(ser, data):
    if ser is not None:
        print_debug(f"SER - {data}")
        try:
            for char in data:
                ser.write(char.encode())
                time.sleep(sleep0)  # Add a 10ms delay between each character
            ser.write(carriage_return.encode())
        except serial.SerialException as e:
            print_error(f"Failed to send data: {e}")
    else:
        print_error("Serial port not open. Cannot send data.")

# **********************************************************************************************
#  Load JSON File
#  Input:
#  - file_path (str): Path to the JSON file
#
#  Output:
#  - dict or None: Loaded JSON data if successful, None otherwise
# **********************************************************************************************
def load_json_file(file_path):
    try:
        with open(file_path, 'r', encoding='utf-8') as json_file:
            data = json.load(json_file)
        return data
    except FileNotFoundError:
        print_error(f"File not found: {file_path}")
        return None
    except json.JSONDecodeError:
        print_error(f"Failed to decode JSON in file: {file_path}")
        return None

# **********************************************************************************************
#  Create Test Description
#  Input:
#  - test_data (dict): Test data dictionary
#
#  Output:
#  - str: Formatted test description
# **********************************************************************************************
def create_test_description(test_data):
    version = test_data.get('version', 'Version Not Available')
    test_name = test_data.get('commandName', 'Test Name Not Available')
    test_description = test_data.get('testDescription', {}).get('text', 'Test Description Not Available')
    command = test_data.get('testDescription', {}).get('command', 'Command Not Available')

    # Process arguments
    arguments = []
    for arg in test_data.get('testDescription', {}).get('arguments', []):
        arg_name = arg.get('name', 'Argument Name Not Available')
        arg_description = arg.get('description', 'Argument Description Not Available')
        if isinstance(arg_name, str):
            arg_name = arg_name.replace(',', '')
        if isinstance(arg_description, str):
            arg_description = arg_description.replace(',', '')
        arguments.append(f"{arg_name} -- {arg_description}")

    arguments = "\n".join(arguments)

    # Process examples
    examples = []
    examples_data = test_data.get('testDescription', {}).get('examples', [])
    for example in examples_data:
        if isinstance(example, dict):
            desc = example.get('description', '')
            cmds = example.get('commands', [])
            if isinstance(desc, str):
                desc = desc.replace(',', '')
            if cmds:
                examples.append(f"{desc}")
                for cmd in cmds:
                    if isinstance(cmd, str):
                        examples.append(f"  {cmd}")
        elif isinstance(example, str):
            examples.append(example.replace(',', ''))

    examples_output = "\n".join(examples)

    # Process comments
    comments = []
    for comment in test_data.get('testDescription', {}).get('comments', []):
        if isinstance(comment, str):
            comment_text = comment.replace(',', '')
            comments.append(comment_text)

    comment_output = "\n".join(comments)
    eeprom = test_data.get('testDescription', {}).get('eeprom', False)

    # Construct final test description text
    test_description_text = f"Command Name: {test_name}\n\n" \
        f"Version:  {version}\n" \
        f"Description:\n{test_description}\n" \
        f"Command:\n{command}\n" \
        f"Arguments:\n{arguments}\n"

    if comment_output:
        test_description_text += f"Comment:\n{comment_output}\n"

    if examples_output:
        test_description_text += f"Examples:\n{examples_output}\n"

    test_description_text += f"Writes to EEPROM: {eeprom}\n"

    return test_description_text

# **********************************************************************************************
#  Clean String
#  Input:
#  - s (str, bytes, or Any): Input to clean
#
#  Output:
#  - str or None: Cleaned string, or None if input is None
# **********************************************************************************************
def clean_string(s):
    if s is None:
        return None
        
    if isinstance(s, bytes):
        decoded = s.decode('utf-8', errors='replace')
        s = decoded
        
    if isinstance(s, str):
        # First check if this is a COM response in JSON format
        json_match = re.search(r'\{"COM\d+"\s*:\s*"([^"]*)"\}', s)
        if json_match:
            s = json_match.group(1)  # Extract just the value part
            
        # Now clean the string
        cleaned = s.replace('>', '').replace('!', '').replace(' ', '').replace('\r', '')
        cleaned = ''.join(c for c in cleaned if c.isprintable() and not c.isspace())
        return cleaned
        
    result = str(s).encode('utf-8')
    return result


# **********************************************************************************************
#  Report Failed Steps
#  Input:
#  - s (str, bytes, or Any): Input to clean
#
#  Output:
#  - str or None: Cleaned string, or None if input is None
# **********************************************************************************************

def report_failed_steps(json_file_path, failed_steps):
    if failed_steps:
        print_warning(f"\nFailed steps in {json_file_path}:")
        for step, description in failed_steps:
            print_warning(f"  Step {step}: {description}")
    else:
        print_info(f"All steps passed in {json_file_path}")



def send_command_wait_for_cursor(ser, command, timeout=2.0):
    """
    Send a command to the serial port and wait for a cursor character ('>' or '!') before returning the response.
    
    Input:
    - ser (serial.Serial): Serial port object
    - command (str): Command to send
    - timeout (float): Maximum time to wait for cursor, in seconds
    
    Output:
    - tuple: (response_text, cursor_char, success)
    """
    # Clear any pending data
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    
    # Send the command
    print_debug(f"Sending command: '{command}'")
    send_serial_string(ser, command)
    
    # Initialize variables
    response = ""
    cursor_char = None
    start_time = time.time()
    
    # Wait for response until timeout or cursor character received
    while time.time() - start_time < timeout:
        if ser.in_waiting > 0:
            chunk = ser.read(ser.in_waiting).decode('ascii', errors='replace')
            response += chunk
            #print_debug(f"Raw received: '{chunk}'")
            
            # Check for cursor characters
            if '>' in chunk:
                cursor_char = '>'
                print_debug(f"Received prompt cursor: '>'")
                break
            elif '!' in chunk:
                cursor_char = '!'
                print_debug(f"Received error cursor: '!'")
                break
        
        time.sleep(sleep1)  # Short delay to prevent CPU hogging
    
    # Handle timeout
    if cursor_char is None:
        print_warning(f"Timeout waiting for cursor after command: '{command}'")
        return response, None, False
    
    return response, cursor_char, True


# **********************************************************************************************
#  Parse Test Procedures
#  Input:
#  - json_data (dict): JSON data containing test procedures
#  - ser (serial.Serial): Serial port object for communication
#
#  Output:
#  - bool: True if all tests passed, False otherwise
# **********************************************************************************************
def parse_test_procedures(json_data, ser):
    port = "COM0"
    global exit_current_test
    file_passed = True
    regex_passed = True
    failed_steps = []  # New list to track failed steps

    if json_data is not None:
        version = json_data.get('version', 'Version Not Available')
        test_procedures = json_data.get('testProcedures', [])
        command_name = json_data.get('commandName', 'Command Name Not Available')
        
        if (Level_DEBUG):
            print_debug(f"Test Procedures from {command_name}:")
            print_debug(f"Version: {version}")
        
        total_steps = len(test_procedures)
        for step, procedure in enumerate(test_procedures, 1):
            if exit_current_test:
                print_debug("Exiting current test...")
                break

            description = procedure.get('description', '')
            comment = procedure.get('comment', '')
            procedure_type = procedure.get('type', '')
            command = procedure.get('command', '')
            ret_cursor = procedure.get('retCursor', '')
            ret_value = procedure.get('retValue', '')
            pValue = procedure.get('pValue', '')
            ret_delay = procedure.get('retDelay', '')

            print_debug(f"Processing step {step}/{total_steps}: {description}")
            step_passed = True  # Reset for each step


            if description != "":
                print_debug("-" * TEST_COLUMN_WIDTH)
                print_debug(f"Description: {description}")
            if comment != "":
                print_comment(f"Comment: {comment}")
            if procedure_type != "":
                print_debug(f"Type: {procedure_type}")
            if command != "":
                print_debug(f"Command: {command}")
            if ret_cursor != "":
                print_debug(f"RetCursor: {ret_cursor}")
            if ret_value != "":
                print_debug(f"RetValue: {ret_value}")
            if ret_delay != "":
                print_debug(f"RetDelay: {ret_delay}")
            if pValue != "":
                print_debug(f"Pause Value: {pValue}")

            if procedure_type in actionList:
                doSerial = True
                ser.reset_input_buffer()
                ser.reset_output_buffer()
                time.sleep(sleep1)  # Short delay after clearing buffers


                try:
                    #######################################
                    # Instruction
                    #######################################
                    if procedure_type == "instruction":
                        if comment:
                            print_comment(comment)
                        doSerial = False  # Skip serial communication

                    #######################################
                    # Command
                    #######################################
                    if procedure_type == "command":
                        try:
                            print_info(f"{Fore.GREEN}    Sending:   {command}{Fore.RESET}{bg.RESET}")
                            # Send the command over serial port
                            full_command = f"{command}"
                            send_serial_string(ser, full_command)
                            
                            # Add delay based on retDelay if present
                            if ret_delay:
                                delay_time = float(ret_delay)
                                print_debug(f"Waiting for {delay_time} seconds before reading response...")
                                time.sleep(delay_time)
                            else:
                                time.sleep(sleep1)
                            
                            # Read response
                            response = ""
                            start_time = time.time()
                            timeout = 2.0  # Default timeout
                            
                            # Keep reading until timeout or no more data
                            while time.time() - start_time < timeout:
                                if ser.in_waiting > 0:
                                    chunk = ser.read(ser.in_waiting).decode('ascii', errors='replace')
                                    response += chunk
                                    
                                    print_debug(f"Response: {response}")
                                    if logging_enabled:
                                        logging.info(f"Response: {strip_color_codes(response)}")

                                time.sleep(sleep1)

                            # Always consider command successful
                            step_passed = True

                        
                        except serial.SerialTimeoutException:
                            print_error("Timeout occurred while waiting for data.")
                            file_passed = False
                            step_passed = False
                        except Exception as e:
                            print_error(f"Error in command procedure: {str(e)}")
                            print_error(traceback.format_exc())
                            file_passed = False
                            step_passed = False


                    #######################################
                    # BinOut
                    #######################################
                    elif procedure_type == "binOut":
                        try:
                            # Split the command string by commas and remove whitespace
                            decimal_values = [int(x.strip()) for x in command.split(',')]
                            # Convert decimal values to bytes
                            binary_data = bytes(decimal_values)
                            print_warning(f"Sending binary data: {binary_data.hex()}")
                            ser.write(binary_data)
                            time.sleep(sleep1)  # Short delay after sending binary data
                            print_warning("Binary data sent successfully")
                        except ValueError as ve:
                            print_error(f"Invalid decimal value in binOut command: {str(ve)}")
                            file_passed = False
                        except serial.SerialException as se:
                            print_error(f"Serial communication error in binOut: {str(se)}")
                            file_passed = False
                    else:
                        print_warning(f"{Fore.LIGHTYELLOW_EX}Description: {description}{Fore.RESET}{bg.RESET}")
                        print_debug(f"{Fore.GREEN}    Sending:   {command}{Fore.RESET}{bg.RESET}")
                        #send_serial_string(ser, f"{port} {command}") #Send out the command
                        full_command = f"{port} {command}"
                        received_data, cursor_value, success = send_command_wait_for_cursor(ser, full_command)
                        
                    # Add delay based on retDelay if present
                    if ret_delay:
                        delay_time = float(ret_delay)
                        print_debug(f"Waiting for {delay_time} seconds before reading response...")
                        time.sleep(delay_time)
                    #else:
                    #    time.sleep(sleep1)


                    #######################################
                    # Quiet
                    #######################################
                    if procedure_type == "quiet":
                        #print_debug("QUIET")
                        received_data = ""
                        # For "quiet" type, just read and discard any data
                        start_time = time.time()
                        while time.time() - start_time < 1.0:  # 1 second timeout
                            if ser.in_waiting > 0:
                                chunk = ser.read(ser.in_waiting).decode('ascii', errors='replace')
                                print_debug(f"Direct read chunk: {chunk}")
                                received_data += chunk
                            time.sleep(0.1)
                        print_debug(f"Total received: {received_data}")

                    #######################################
                    # Binary
                    #######################################
                    elif procedure_type == "binary":
                        try:
                            print_debug(f"Processing binary response: {received_data.encode('ascii', 'ignore').hex()}")
                            
                            # Improved binary data extraction approach
                            # First, encode the response to bytes
                            response_bytes = received_data.encode('ascii', 'ignore')
                            
                            # Create a more robust approach to extract binary data
                            # Use regular expression to find binary sequences
                            binary_matches = re.findall(rb'[\x00-\xff]+', response_bytes)
                            
                            if binary_matches:
                                # Pick the longest binary sequence as the most likely valid data
                                received_binary = max(binary_matches, key=len)
                                print_debug(f"Extracted binary data: {received_binary.hex()}")
                                
                                # Convert the expected value from hex string to bytes
                                expected_binary = bytes.fromhex(ret_value)
                                
                                # Compare with expected binary data
                                if received_binary == expected_binary:
                                    print_warning("Binary test passed.")
                                else:
                                    # More detailed failure information
                                    print_warning(f"{bg.WHITE}{Fore.RED}Binary test failed.{Fore.RESET}{bg.RESET}")
                                    print_warning(f"Expected: {expected_binary.hex()}")
                                    print_warning(f"Received: {received_binary.hex()}")
                                    print_warning(f"Length: Expected {len(expected_binary)} bytes, Got {len(received_binary)} bytes")
                                    
                                    # Show differences byte by byte for easier debugging
                                    min_len = min(len(expected_binary), len(received_binary))
                                    for i in range(min_len):
                                        if expected_binary[i] != received_binary[i]:
                                            print_warning(f"Difference at position {i}: Expected 0x{expected_binary[i]:02x}, Got 0x{received_binary[i]:02x}")
                                    
                                    file_passed = False
                                    step_passed = False
                            else:
                                print_warning(f"{bg.WHITE}{Fore.RED}No binary data found in response{Fore.RESET}{bg.RESET}")
                                print_warning(f"Raw response: {response_bytes.hex()}")
                                file_passed = False
                                step_passed = False
                        except Exception as e:
                            print_error(f"Error in binary test: {str(e)}")
                            print_error(f"Error type: {type(e).__name__}")
                            print_error(f"Error traceback: {traceback.format_exc()}")
                            file_passed = False
                            step_passed = False


                    #######################################
                    # Device Specific
                    #######################################
                    elif procedure_type == "device_specific":
                        # Extract JSON objects from the received data
                        extracted_values = ""
                        json_objects = re.findall(r'\{.*?\}', received_data)
                        
                        # Process each JSON object more robustly
                        for json_obj in json_objects:
                            try:
                                # First attempt to properly parse the JSON
                                try:
                                    parsed_obj = json.loads(json_obj)
                                    # If it's in the expected format with COM key
                                    com_keys = [key for key in parsed_obj.keys() if key.startswith('COM')]
                                    if com_keys:
                                        com_key = com_keys[0]
                                        value = parsed_obj[com_key]
                                        if isinstance(value, str):
                                            extracted_values += value.replace(" ", "")  # Remove spaces
                                        else:
                                            print_debug(f"Value for {com_key} is not a string: {value}")
                                except json.JSONDecodeError:
                                    # Fallback to regex if JSON parsing fails
                                    print_debug(f"JSON parsing failed, using regex fallback for: {json_obj}")
                                    match = re.search(r'\{"COM\d+"\s*:\s*"([^"]*)"\}', json_obj)
                                    if match:
                                        value = match.group(1)
                                        extracted_values += value.replace(" ", "")  # Remove spaces
                                    else:
                                        print_debug(f"Regex extraction failed for: {json_obj}")
                            except Exception as e:
                                print_debug(f"Error processing JSON object: {json_obj}")
                                print_debug(f"Error details: {str(e)}")
                                # Continue processing other JSON objects even if one fails
                        
                        # Use extracted values if available, otherwise use entire response
                        cleaned_received = clean_string(extracted_values) if extracted_values else clean_string(received_data)
                        
                        # Get both the original and alternative expected values from the config
                        expected_value, alt_expected_value = get_config_value(identity, ret_value)
                        cleaned_expected = clean_string(expected_value) if expected_value is not None else None
                        cleaned_alt_expected = clean_string(alt_expected_value) if alt_expected_value is not None else None

                        # Improved comparison logic with better error messages
                        if expected_value is None and alt_expected_value is None:
                            print_debug(f"No matching config value found for '{ret_value}' or '{ret_value}_alt'")
                            print_warning(f"{bg.WHITE}{Fore.RED}Test failed: Missing expected value configuration{Fore.RESET}{bg.RESET}")
                            step_passed = False
                        elif cleaned_received == cleaned_expected:
                            print_debug(f"Comparison passed with original value for '{ret_value}'")
                            print_debug(f"Received data: {repr(cleaned_received)}")
                            print_debug(f"Expected data: {repr(cleaned_expected)}")
                        elif cleaned_received == cleaned_alt_expected:
                            print_debug(f"Comparison passed with alternate value for '{ret_value}_alt'")
                            print_debug(f"Received data: {repr(cleaned_received)}")
                            print_debug(f"Alternate expected data: {repr(cleaned_alt_expected)}")
                        else:
                            print_warning(f"{bg.WHITE}{Fore.RED}Comparison FAILED{Fore.RESET}{bg.RESET}")
                            print_warning(f"{bg.WHITE}{Fore.RED}Received data: {repr(cleaned_received)}{Fore.RESET}{bg.RESET}")
                            if cleaned_expected is not None:
                                print_warning(f"{bg.WHITE}{Fore.RED}Expected value: {repr(cleaned_expected)}{Fore.RESET}{bg.RESET}")
                            if cleaned_alt_expected is not None:
                                print_warning(f"{bg.WHITE}{Fore.RED}Alternate value: {repr(cleaned_alt_expected)}{Fore.RESET}{bg.RESET}")
                            step_passed = False

                    #######################################
                    # Regex
                    #######################################
                    elif procedure_type == "regex":
                        # Perform regex match on the complete response received up to cursor
                        pattern = re.compile(ret_value)
                        match = pattern.search(received_data)

                        if match:
                            print_warning(f"Regex match: {match.group()}")
                        else:
                            print_warning(f"{bg.WHITE}{Fore.RED}Regex match not found. Pattern: '{ret_value}', Received: '{received_data}'{Fore.RESET}{bg.RESET}")
                            file_passed = False
                            regex_passed = False
                            step_passed = False

                        if cursor_value != ret_cursor and ret_cursor:
                            print_warning(f"{bg.WHITE}{Fore.RED}Test failed: Cursor mismatch. Expected '{ret_cursor}', got '{cursor_value}'{Fore.RESET}{bg.RESET}")
                            file_passed = False
                            regex_passed = False
                            step_passed = False

                    #######################################
                    # Complex or Simple
                    #######################################
                    elif procedure_type == "complex" or procedure_type == "simple":
                        # Get cleaned strings for comparison
                        cleaned_received = clean_string(received_data)
                        cleaned_expected = clean_string(ret_value)
                        # Compare and handle results
                        if cleaned_received != cleaned_expected:
                            file_passed = False
                            step_passed = False
                            if verbose >= 2:
                                print_warning(f"{bg.WHITE}{Fore.RED}{procedure_type.capitalize()} comparison FAILED{Fore.RESET}{bg.RESET}")
                                print_warning(f"Received data: '{cleaned_received}'")
                                print_warning(f"Expected data: '{cleaned_expected}'")
                        else:
                            print_warning(f"{procedure_type.capitalize()} comparison passed")

                        if ret_cursor and cursor_value != ret_cursor:
                            file_passed = False
                            step_passed = False    
                            if verbose >= 2:
                                print_warning(f"{bg.WHITE}{Fore.RED}Cursor mismatch. Expected '{ret_cursor}', got '{cursor_value}'{Fore.RESET}{bg.RESET}")

                except serial.SerialTimeoutException:
                    print_error("Timeout occurred while waiting for data.")
                    file_passed = False

                except KeyboardInterrupt:
                    print_comment("Exiting...")
                    exit_current_test = True
                    file_passed = False

            if pValue:
                try:
                    numeric_pValue = int(pValue)
                    print_debug(">>PVALUE = " + str(numeric_pValue))
                    time.sleep(numeric_pValue)
                except ValueError:
                    print_error("Error: pValue is not a valid number.")

                if verbose > 0:
                    print_info(f"Pause")
                    #print_info("-" * TEST_COLUMN_WIDTH)


            if not step_passed:
                file_passed = False
                failed_steps.append((step, description))
                print_warning(f"Step failed: {description}")

    return file_passed, failed_steps

# **********************************************************************************************
#  Process Master File
#  Input:
#  - file_path (str): Path to the master file
#  - base_path (str): Base path for relative file paths (default: '')
#
#  Output:
#  - tuple: (list of JSON file paths, master version)
# **********************************************************************************************
def process_master_file(file_path, base_path=''):
    json_files = []
    master_version = None

    try:
        with open(file_path, 'r', encoding='utf-8') as master_file:
            master_data = json.load(master_file)
            master_version = master_data.get('version', 'Unknown')

            for file_name in master_data.get("json_files", []):
                full_path = os.path.join(base_path, file_name)
                if file_name.endswith('.json'):
                    json_files.append((full_path, master_version))
                elif file_name.endswith('.master'):
                    nested_json_files, _ = process_master_file(full_path, os.path.dirname(full_path))
                    json_files.extend(nested_json_files)

    except Exception as e:
        print_error(f"Error processing {file_path}: {str(e)}")

    return json_files, master_version

# **********************************************************************************************
#  List JSON Files
#  Input:
#  - directory (str, optional): Directory to search for JSON files
#  - input_path (str, optional): Path to a specific JSON or master file
#
#  Output:
#  - tuple: (list of JSON file paths, master version)
# **********************************************************************************************
def list_json_files(directory=None, input_path=None):
    json_files = []
    master_version = 'Unknown'
    
    if input_path:
        base_path = os.path.dirname(input_path)
        if input_path.endswith('.master'):
            master_version = read_master_version(input_path)
            json_files, _ = process_master_file(input_path, base_path)
        elif input_path.endswith('.json'):
            json_files.append(input_path)
    elif directory:
        for f in os.listdir(directory):
            full_path = os.path.join(directory, f)
            if f.endswith('.json'):
                json_files.append(full_path)
            elif f.endswith('.master'):
                master_version = read_master_version(full_path)
                nested_files, _ = process_master_file(full_path, directory)
                json_files.extend(nested_files)

    return json_files, master_version

# **********************************************************************************************
#  List Master Files
#  Input:
#  - directory (str, optional): Directory to search for master files
#  - input_path (str, optional): Path to a specific master file
#
#  Output:
#  - list: List of master file paths
# **********************************************************************************************
def list_master_files(directory=None, input_path=None):
    global fileCnt
    master_files = []

    def process_damaster_file(file_path):
        global fileCnt
        #print_debug("Master file list")
        if file_path.endswith('.master'):
            #print_debug(file_path)
            master_files.append(file_path)
            fileCnt += 1

            # Read the master file and process any nested master files
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                    nested_files = data.get("full_list", [])
                    for nested_file in nested_files:
                        # Construct the full path for the nested file
                        nested_file_path = os.path.join(os.path.dirname(file_path), nested_file)
                        if nested_file.endswith('.master'):
                            process_damaster_file(nested_file_path)
            except Exception as e:
                print_error(f"Error processing master file {file_path}: {e}")

    if input_path:
        process_damaster_file(input_path)
    elif directory:
        for f in os.listdir(directory):
            full_path = os.path.join(directory, f)
            process_damaster_file(full_path)

    return master_files

# **********************************************************************************************
#  Print Descriptions
#  Input:
#  - json_files (list): List of JSON file paths to process
#
#  Output:
#  - None (prints to console)
# **********************************************************************************************
def print_descriptions(json_files):
    for json_file_tuple in json_files:
        json_file_path = json_file_tuple[0] if isinstance(json_file_tuple, tuple) else json_file_tuple
        json_data = load_json_file(json_file_path)
        print_info("-" * TEST_COLUMN_WIDTH + "\n")
        if json_data:
            test_description = create_test_description(json_data)
            print_info(f"Test Description from {json_file_path}:\n")
            print_info(test_description)

# **********************************************************************************************
#  Print No Process Files
#  Input:
#  - master_files (list): List of master file paths
#
#  Output:
#  - None (prints to console)
# **********************************************************************************************
def print_noProcessFiles(master_files):
    # List to keep track of master files that have entries in the "not_processed" list
    files_with_unprocessed = []
    aggregated_unprocessed_entries = []

    # Iterate over each master file provided in the list
    for master_file in master_files:
        try:
            # Open and load the master file
            with open(master_file, 'r') as file:
                data = json.load(file)

            # Check if there are any entries in the "not_processed" list
            not_processed = data.get("not_processed", [])
            if not_processed:
                # If the "not_processed" list is not empty, add the file to the list
                files_with_unprocessed.append(master_file)
                aggregated_unprocessed_entries.extend(not_processed)
        except FileNotFoundError:
            print_error(f"File not found: {master_file}")
        except json.JSONDecodeError:
            print_error(f"Error decoding JSON from file: {master_file}")
        except Exception as e:
            print_error(f"An error occurred while processing {master_file}: {e}")

    # Print the aggregated list of "not_processed" entries from all master files
    if aggregated_unprocessed_entries:
        print_info("List of 'not_processed' entries from all master files:")
        for entry in aggregated_unprocessed_entries:
            print_info(f" - {entry}")
    else:
        print_info("'not_processed' entries NOT found in any master files.")

# **********************************************************************************************
#  Write Description to File
#  Input:
#  - file_path (str): Path to the output file
#  - content (str): Content to write to the file
#
#  Output:
#  - None (writes to file)
# **********************************************************************************************
def write_description_to_file(file_path, content):
    with open(file_path, 'a', encoding='utf-8') as f:
        f.write(content + '\n')

# **********************************************************************************************
#  Process Master File for Descriptions
#  Input:
#  - file_path (str): Path to the master file
#  - output_file (str): Path to the output file
#
#  Output:
#  - None (processes files and writes descriptions)
# **********************************************************************************************
def process_master_file_for_descriptions(file_path, output_file):
    try:
        with open(file_path, 'r', encoding='utf-8') as master_file:
            master_data = json.load(master_file)
            base_path = os.path.dirname(file_path)
            for file_name in master_data.get("json_files", []):
                full_path = os.path.join(base_path, file_name)
                if file_name.endswith('.json'):
                    process_json_file(full_path, output_file)
                elif file_name.endswith('.master'):
                    process_master_file_for_descriptions(full_path, output_file)
    except Exception as e:
        write_description_to_file(output_file, f"Error processing {file_path}: {str(e)}")

# **********************************************************************************************
#  Print Non-Test Procedure Sections
#  Input:
#  - json_file_path (str): Path to the JSON file
#
#  Output:
#  - None (prints to console)
# **********************************************************************************************
def print_non_test_procedure_sections(json_file_path):
    print_debug(f"Attempting to print description for: {json_file_path}")
    try:
        json_data = load_json_file(json_file_path)
        if json_data:
            print_info(f"\nDescription for {json_file_path}:")
            print_info("-" * TEST_COLUMN_WIDTH)
            test_description = create_test_description(json_data)
            print_info(test_description)
            print_info("=" * TEST_COLUMN_WIDTH)
        else:
            print_error(f"No data found in {json_file_path}")
    except FileNotFoundError:
        print_error(f"File not found: {json_file_path}")
    except Exception as e:
        print_error(f"Error processing {json_file_path}: {str(e)}")

# **********************************************************************************************
#  Process JSON File
#  Input:
#  - json_file_path (str): Path to the JSON file
#  - output_file (str): Path to the output file
#
#  Output:
#  - None (processes file and writes description to output file)
# **********************************************************************************************
def process_json_file(json_file_path, output_file):
    try:
        json_data = load_json_file(json_file_path)
        if json_data:
            description = f"\nDescription for {json_file_path}:\n"
            description += "-" * TEST_COLUMN_WIDTH + "\n"
            description += create_test_description(json_data)
            description += "=" * TEST_COLUMN_WIDTH + "\n"
            write_description_to_file(output_file, description)
        else:
            write_description_to_file(output_file, f"No data found in {json_file_path}")
    except FileNotFoundError:
        write_description_to_file(output_file, f"File not found: {json_file_path}")
    except Exception as e:
        write_description_to_file(output_file, f"Error processing {json_file_path}: {str(e)}")

# **********************************************************************************************
#  Process Tests
#  Input:
#  - json_files (list): List of JSON file paths to process
#
#  Output:
#  - tuple: (start_time, end_time, duration, pass_count, fail_count)
# **********************************************************************************************
def process_tests(json_files):
    global exit_program, exit_current_test

    ser = None
    start_time = datetime.now()

    pass_count = 0
    fail_count = 0

    if verbose > 0:
        print_debug(f"Tests started at: {start_time.strftime('%Y-%m-%d %H:%M')}")

    if verbose == 0:
        print_info("")
        print_info(f"{'Test Name':<{TEST_NAME_WIDTH}} | Ver | Pass | Fail | Master")
        print_info("-" * (TEST_COLUMN_WIDTH + 7))  # Increased width for master version


    try:
        ser = open_serial_port(config)
        if ser is None:
            print_info(f"\nERROR OPENING SERIAL PORT -- config.json -- {config}\n")
            raise ValueError("Failed to open serial port!")

        for json_file_path, master_version in json_files:
            if exit_program:
                break

#            if verbose > 0:
            print_comment("=" * TEST_COLUMN_WIDTH)
            print_comment(f"File: {json_file_path}")
            
            try:
                json_data = load_json_file(json_file_path)
                if json_data:
                    exit_current_test = False
                    file_passed, failed_steps  = parse_test_procedures(json_data, ser)

                test_name = os.path.basename(json_file_path)[:TEST_NAME_WIDTH].ljust(TEST_NAME_WIDTH)
                version = json_data.get('version', 'N/A')[:3]  # Get first 3 characters of version

                if file_passed:
                    pass_count += 1
                    print_debug(f"Pass count increased to {pass_count}")
                    print_comment(f"PASSED -- File {json_file_path}")
                    print_info(f"{test_name:<{TEST_NAME_WIDTH}} | {version:3} |  P   |      | {master_version}")
                else:
                    fail_count += 1
                    print_debug(f"Fail count increased to {fail_count}")
                    print_comment(f"{bg.WHITE}{Fore.RED}FAILED -- File {json_file_path}{Fore.RESET}{bg.RESET}")
                    report_failed_steps(json_file_path, failed_steps)                        
                    print_info(f"{bg.WHITE}{Fore.RED}{test_name:<{TEST_NAME_WIDTH}} | {version:3} |      |  F   | {master_version}   {Fore.RESET}{bg.RESET}")
            
            except Exception as e:
                print_error(f"Error processing file {json_file_path}: {str(e)}")
                print_error(f"Error type: {type(e).__name__}")
                print_error(f"Error traceback: {traceback.format_exc()}")
                                
    except Exception as e:
        error_message = f"System Error: {str(e)}"
        print_error(error_message)
        print_error(f"Error type: {type(e).__name__}")
        print_error(f"Error traceback: {traceback.format_exc()}")
    finally:
        if ser:
            close_serial_port(ser)
        
        end_time = datetime.now()
        duration = end_time - start_time
        
        if verbose == 0:
            print_info("-" * (TEST_COLUMN_WIDTH + 7))
            print_info(f"Total: Pass = {pass_count}, Fail = {fail_count}")

    if verbose > 0:
        print_comment("=" * TEST_COLUMN_WIDTH)

    return start_time, end_time, duration, pass_count, fail_count

# ***********************************************
#  Main
# ***********************************************
if __name__ == "__main__":
    program_description = f"""G1 Plus Tester v{codeVersion}
"""
    
    parser = argparse.ArgumentParser(
        description=program_description,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )    
    
    parser.add_argument("input_path", type=str, nargs='?',
                       help="Path to the .master file or a single .json file")
    
    parser.add_argument("-c", "--config", help="Path to / name of configuration file", default="config.json")

    parser.add_argument("-d", "--description", action="store_true", help="Print out the Test Descriptions.")
    parser.add_argument("-dir", "--directory", help="Directory containing JSON files.", default=None)
    
    parser.add_argument("-i", "--identity", help="Path to / name of device identity file", default="identity.json")

    # New device configuration arguments
    parser.add_argument("-dc", "--device-config", help="Path to device configuration JSON file")
    parser.add_argument("--save-config", action="store_true", help="Save the configuration to device memory after applying")

    parser.add_argument("-l", "--log", action="store_true", help="Enable logging.")
    parser.add_argument("-ld", "--log-details", action="store_true", help="Enable detailed logging with serial number, date, time, and test result.")
    parser.add_argument("-lf", "--log-file", type=str, default="test.log", help="Log file name - default is 'test.log'.  Only used when '-l' is enabled.")

    parser.add_argument("-sn", "--serial-number", type=str, help="Serial number to be included in the log file name.")

    parser.add_argument("-nc", "--no-color", action="store_true", help="Disable colored output to the console. Color is disabled when writing to log files.")
    parser.add_argument("-np", "--noprocess", action="store_true", help="Scan all master files and list files not processed.")
 
    parser.add_argument("-v", "--verbose", type=int, default=0, help="Enable verbose mode - v0=simple pass/fail by file.  v1=TBD.  v2=step by step pass/fail.  v3=debug -- uses dB, dC, dH, dS, ndB, ndC, ndH, ndS flags.  Any combination of these flags may be used.")

    parser.add_argument("-dB", "--dispBytes", action="store_true", help="Display expected and actual return values in byte format.  Only used when -v3 is enabled. Default is OFF.")
    parser.add_argument("-dC", "--dispCurs", action="store_true", help="Display expected and actual cursor values.  Only used when -v3 is enabled. Default is OFF.")
    parser.add_argument("-dH", "--dispHex", action="store_true", help="Display expected and actual return values in hex format.  Only used when -v3 is enabled. Default is OFF.")
    parser.add_argument("-dS", "--dispStr", action="store_true", help="Display expected and actual return values in string format.  Only used when -v3 is enabled. Default is ON.")
    parser.add_argument("-ndB", "--nodispBytes", action="store_true", help="Do not display expected and actual return values in byte format.  Only used when -v3 is enabled. Default is OFF.")
    parser.add_argument("-ndC", "--nodispCurs", action="store_true", help="Do not display expected and actual cursor values.  Only used when -v3 is enabled. Default is OFF.")
    parser.add_argument("-ndH", "--nodispHex", action="store_true", help="Do not display expected and actual return values in hex format.  Only used when -v3 is enabled. Default is OFF.")
    parser.add_argument("-ndS", "--nodispStr", action="store_true", help="Do not display expected and actual return values in string format.  Only used when -v3 is enabled. Default is ON.")

    try:
        import shlex
        argv = sys.argv[1:]
        args = parser.parse_args(argv)
    except Exception as e:
        print_error(f"Error parsing arguments: {str(e)}")
        sys.exit(1)

    # Normalize paths after parsing
    if args.input_path:
        args.input_path = os.path.normpath(args.input_path)
    if args.config:
        args.config = os.path.normpath(args.config)
    if args.identity:
        args.identity = os.path.normpath(args.identity)
    if args.directory:
        args.directory = os.path.normpath(args.directory)
    if args.device_config:
        args.device_config = os.path.normpath(args.device_config)

    log_file_name = setup_logging(args.log or args.log_details, args.log_file, args.verbose, args.serial_number, args.log_details)

    logging_enabled = args.log
    color_enabled = not args.no_color

    verbose = args.verbose
    changeVerbosity(verbose)
 
    if args.dispBytes:
        dispBytes = True

    if args.dispCurs:
        dispCurs = True

    if args.dispHex:
        dispHex = True

    if args.dispStr:
        dispStr = True

    if args.nodispBytes:
        dispBytes = False

    if args.nodispCurs:
        dispCurs = False

    if args.nodispHex:
        dispHex = False

    if args.nodispStr:
        dispStr = False

    # Load main configuration
    if args.config:
        if args.config != 'config.json':
            config = load_config(args.config)
        else:
            config = load_config()

    # Load identity configuration
    if args.identity:
        if args.identity != 'identity.json':
            identity = load_identity(args.identity)
        else:
            identity = load_identity()

    # Print identity data
    print_identity_data(identity)

    if logging_enabled:
        print_debug(f"Writing to {log_file_name}")

    try:
        # Handle device configuration if provided
        if args.device_config:
            success = process_device_config(args.device_config)
            if not success:
                print_error("Failed to process device configuration.")
                if not args.input_path and not args.noprocess and not args.description:
                    sys.exit(1)  # Exit if only device configuration was requested
        
        if args.noprocess:
            master_files = list_master_files(directory=args.directory, input_path=args.input_path)
            if not master_files:
                print_error("No .master files found. Please check your inputs.")
                sys.exit(1)
            else:    
                print_noProcessFiles(master_files)

        elif args.description:
            output_file = "description.txt"
            # Clear the file if it already exists
            open(output_file, 'w').close()
            
            if args.input_path:
                if args.input_path.endswith('.master'):
                    process_master_file_for_descriptions(args.input_path, output_file)
                elif args.input_path.endswith('.json'):
                    process_json_file(args.input_path, output_file)
                else:
                    write_description_to_file(output_file, "Invalid file type. Please provide a .master or .json file.")
            elif args.directory:
                master_files = [f for f in os.listdir(args.directory) if f.endswith('.master')]
                for master_file in master_files:
                    process_master_file_for_descriptions(os.path.join(args.directory, master_file), output_file)
                if not master_files:
                    json_files = [f for f in os.listdir(args.directory) if f.endswith('.json')]
                    for json_file in json_files:
                        process_json_file(os.path.join(args.directory, json_file), output_file)
            else:
                write_description_to_file(output_file, "Please provide a .master file, a .json file, or a directory when using the -d flag.")
            
            print_info(f"Descriptions have been written to {output_file}")


        elif args.input_path or args.directory:
            # Run tests if input path or directory is provided
            if color_enabled and verbose > 0:
                print_info(bg.BLACK)  # Set the color, if enabled.

            changeVerbosity(args.verbose)

            #else:
            json_files, master_version = list_json_files(directory=args.directory, input_path=args.input_path)
            if not json_files:
                print_error("No JSON files found. Please check your inputs.")
                sys.exit(1)

            # Initialize summary variables
            start_time = end_time = datetime.now()
            duration = timedelta(0)
            passed = failed = 0

            start_time, end_time, duration, passed, failed = process_tests(json_files)

            print_info("\nTest Execution Summary:")
            print_info(f"Start Time: {start_time.strftime('%Y-%m-%d %H:%M:%S')}")
            print_info(f"End Time: {end_time.strftime('%Y-%m-%d %H:%M:%S')}")
            print_info(f"Total Duration: {duration}")
            print_info(f"Files Passed: {passed}")
            print_info(f"Files Failed: {failed}")
            print_info(f"Total Files: {passed + failed}")

            if args.log_details and log_file_name:
                test_result = "PASS" if (passed > 0 and failed == 0) else "FAIL"
                file_name, file_extension = os.path.splitext(log_file_name)
                new_log_file_name = f"{file_name}_{test_result}{file_extension}"
                os.rename(log_file_name, new_log_file_name)
                print_info(f"Updated log file name: {new_log_file_name}")

    except SystemExit:
        print_error(f"Program terminated by user.")
    except Exception as e:
        print_error(f"An unexpected error occurred: {str(e)}")
        traceback.print_exc()
    finally:
        print_debug("Cleaning up and closing any open resources...")
        # Add any necessary cleanup code here

    # Final message about how to exit
    if sys.platform == "win32":
        print_debug("Press Ctrl+Break to skip the current test.")

print_debug("Closing")
