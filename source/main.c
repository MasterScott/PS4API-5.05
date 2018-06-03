#include "ps4.h"
#include "include/defines.h"
#include "include/global.h"
#include "include/kernel.h"
#include "include/sock.h"
#include "include/process.h"
#include "include/commandHandlers.h"
#define VERSION "1.2"


#define MSG_CLIENT_CONNECED "Cliente [%s] conectado" 
#define MSG_CLIENT_DISCONNECED "Cliente [%s] desconectado"
#define MSG_CLIENT_THREAD_ERROR "Erro handling the client"

int createThread(void*(func)(void*), void* args)
{
	ScePthread sceThread;
	return scePthreadCreate(&sceThread, NULL, func, args, "Clien Thread") == 0;
}

void* clientHandler(void* args)
{
	struct sockaddr_in client = *(struct sockaddr_in*)args;
	
	int locClientSocketFd = clientSockFd;
	char clientIP[16];// Pro IPv4
	bool gotUnknownCommand = true;
	command_s *localCommands = commands;
	int localCommandsLength = lenOfCommands;
	clientIp(&client.sin_addr, clientIP);
	NOTIFY(MSG_CLIENT_CONNECED, clientIP);
	for (INFINITE)
	{
			char bufferOfClient[MAX_RECEIVE_LENGTH] = {0};
			int lenOfReceivedData = receiveFromClient(locClientSocketFd, bufferOfClient, MAX_RECEIVE_LENGTH);
			
			if (lenOfReceivedData < 1)
			{
				NOTIFY(MSG_CLIENT_DISCONNECED, clientIP);
				scePthreadExit(NULL);
			}

			if (bufferOfClient[0] == 'q')
			{
				quitCommandHandler();
				closeSocket(locClientSocketFd);
				NOTIFY(MSG_CLIENT_DISCONNECED, clientIP);
				scePthreadExit(NULL);
			}
			for (size_t i = 0; i < localCommandsLength; i++) 
			{
		    	if(localCommands[i].commandChar == bufferOfClient[0] && localCommands[i].minLength <= lenOfReceivedData  && localCommands[i].handler != NULL)
		        {
		            localCommands[i].handler(bufferOfClient, lenOfReceivedData);
					gotUnknownCommand = false;
					break;
		        }
		    }
			if (gotUnknownCommand){
				unknownCommandHandler();
			}
			gotUnknownCommand = true;
	}
	return NULL;
}

int init(){
	initKernel();
	initLibc();
	kexec(kernelPayload, NULL);
	initNetwork();
	initSysUtil();
	initPthread();
	
	if (initSockets())
	{
		NOTIFY("PS4 API by BISOON ported to 5.05 by @Helloitu v%s\n", VERSION);
		return 1;
	}
	else
	{
		NOTIFY("PS4API: Failed to run the port not available, try again later\n");
		closeSockets();
		return 0;
	}
}
int _main(void) {
	
	if (!init())
		return 1;
	PRINTS("INICIADO");
	struct sockaddr_in clientStruct;
	int clientSocketMonitor = -1;
	for (INFINITE) 
	{
		
		clientSockFd = acceptClient(&clientStruct);
		if (clientSocketMonitor != -1)
		{
			abortSendRecv(clientSocketMonitor);
			closeSocket(clientSocketMonitor);
		}
		clientSocketMonitor = clientSockFd;
		if (!createThread(clientHandler, &clientStruct))
			NOTIFY(MSG_CLIENT_THREAD_ERROR);
	}
	closeSockets();
	return 0;
}
