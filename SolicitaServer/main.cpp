#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <sstream>

#pragma comment (lib, "ws2_32.lib")

using namespace std;

int main()
{
	// Inicializa WinSock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0)
	{
		cerr << "Falha ao inicializar winsock! Finalizando..." << endl;
		return 99;
	}

	// Cria o socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET)
	{
		cerr << "Falha ao criar o socket! Finalizando..." << endl;
		return 99;
	}

	// Bind the ip address and port to a socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000);
	hint.sin_addr.S_un.S_addr = INADDR_ANY; // Could also use inet_pton .... 

	bind(listening, (sockaddr*)&hint, sizeof(hint));

	// Tell Winsock the socket is for listening 
	listen(listening, SOMAXCONN);

	// Create the master file descriptor set and zero it
	fd_set master;
	FD_ZERO(&master);

	// Add our first socket that we're interested in interacting with; the listening socket!
	// It's important that this socket is added for our server or else we won't 'hear' incoming
	// connections 
	FD_SET(listening, &master);

	// this will be changed by the \quit command (see below, bonus not in video!)
	bool running = true;

	cout << "TCP/IP CHAT SERVER v0.0.1" << endl;

	while (running)
	{
		fd_set copy = master;

		// See who's talking to us
		int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

		// Loop entre todas as conexões  / conexoes em potencial
		for (int i = 0; i < socketCount; i++)
		{
			// facilita
			SOCKET sock = copy.fd_array[i];

			// Se tiver uma conexão vindo 
			if (sock == listening)
			{
				// Aceita a nova conexão
				SOCKET client = accept(listening, nullptr, nullptr);

				// Adiciona conexão a lista de clientes conectados
				FD_SET(client, &master);

				// Envia uma mensagem de boas vindas ao cliente conectado (desabilitado)
				//string welcomeMsg = "";
				//send(client, welcomeMsg.c_str(), welcomeMsg.size() + 1, 0);
			}
			else // It's an inbound message
			{
				char buf[4096];
				ZeroMemory(buf, 4096);

				// Recebe mensagem
				int bytesIn = recv(sock, buf, 4096, 0);
				if (bytesIn <= 0)
				{
					// Drop the client
					closesocket(sock);
					FD_CLR(sock, &master);
				}
				else
				{
					// Envia mensagem para todos os outros clientes, e não para o atual

					for (u_int i = 0; i < master.fd_count; i++)
					{
						SOCKET outSock = master.fd_array[i];
						if (outSock == listening)
						{
							continue;
						}

						ostringstream ss;

						ss << buf;										

						string strOut = ss.str();
						send(outSock, strOut.c_str(), strOut.size() + 1, 0);
					}
				}
			}
		}
	}

	// Remove the listening socket from the master file descriptor set and close it
	// to prevent anyone else trying to connect.
	FD_CLR(listening, &master);
	closesocket(listening);

	// Message to let users know what's happening.
	string msg = "SERVER:Server is shutting down. Goodbye\r\n";

	while (master.fd_count > 0)
	{
		// Get the socket number
		SOCKET sock = master.fd_array[0];

		// Send the goodbye message
		send(sock, msg.c_str(), msg.size() + 1, 0);

		// Remove it from the master file list and close the socket
		FD_CLR(sock, &master);
		closesocket(sock);
	}

	// Cleanup winsock
	WSACleanup();

	system("pause");
	return 0;
}