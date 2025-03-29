/*
 * command.h
 *
 *  Created on: Feb 17, 2025
 *      Author: mavorpdx
 */

#ifndef INC_COMMAND_H_
#define INC_COMMAND_H_

extern void processSerialCommand(char* command, char* data);
extern void sendReply(const char* from, const char* message);
extern void sendDebug(const char* from, const char* message);

void printHelp(char *buffer);
void printSystemStatus(char *buffer);


#endif /* INC_COMMAND_H_ */
