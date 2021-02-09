#
/*
 *	Copyright (C) 2020
 *	Jan van Katwijk (J.vanKatwijk@gmail.com)
 *	Lazy Chair Computing
 *
 *	This file is part of the SDRunoPlugin_1090
 *
 *	The "parsing" and the building of the response are taken from
 *	dump1090,
 *	Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>

 *    SDRunoPlugin_1090 is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDRunoPlugin_1090 is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDRunoPlugin_1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<sys/types.h> 
#include	<winsock2.h>
#include	<windows.h>
#include	<ws2tcpip.h>
#include	<cstring>
#pragma comment(lib, "Ws2_32.lib")
#include	"http-handler.h"
#include	"SDRunoPlugin_1090.h"
#include	"aircraft-handler.h"

	httpHandler::httpHandler (SDRunoPlugin_1090 *parent,
	                                        std::string mapFile) {
	this	-> parent	= parent;
	this	-> mapFile	= mapFile;
	this	-> running. store (false);
}

	httpHandler::~httpHandler	() {
	if (running. load ()) {
	   running. store (false);
	   threadHandle. join ();
	}
}

void	httpHandler::start	() {
	threadHandle = std::thread (&httpHandler::run, this);
}

void	httpHandler::stop	() {
	if (running. load ()) {
           running. store (false);
           threadHandle. join ();
        }
}

void	httpHandler::run	() {
char	buffer [4096];
bool	keepalive;
char	*url;
std::string	content;
std::string	ctype;
WSADATA	wsa;
int	iResult;
SOCKET	ListenSocket	= INVALID_SOCKET;
SOCKET	ClientSocket	= INVALID_SOCKET;

struct addrinfo *result = NULL;
struct addrinfo hints;

	if (WSAStartup (MAKEWORD (2, 2), &wsa) != 0)
	   return;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

//	Resolve the server address and port
	iResult = getaddrinfo (NULL, "8080", &hints, &result);
	if (iResult != 0 ) {
           WSACleanup();
	   parent -> show_text ("getaddr fails\n");
	   return;
	}

// Create a SOCKET for connecting to server
	ListenSocket = socket (result->ai_family,
	                       result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
	   parent -> show_text ("socket failed");
	   freeaddrinfo(result);
	   WSACleanup ();
	   return;
	}

// Setup the TCP listening socket
	iResult = bind( ListenSocket, result->ai_addr,
	                                (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
	   parent -> show_text ("bind failed");
	   freeaddrinfo (result);
	   closesocket (ListenSocket);
	   WSACleanup ();
	   return;
	}

	freeaddrinfo (result);

	running. store (true);
	listen (ListenSocket, 5);
	while (running. load ()) {
	   ClientSocket = accept (ListenSocket, NULL, NULL);
	   if (ClientSocket == -1)  
	      continue;

	   while (running) {
	      if (recv(ClientSocket, buffer, 4096, 0) < 0) {
// shutdown the connection since we're done
	         iResult = shutdown(ClientSocket, SD_SEND);
	         if (iResult == SOCKET_ERROR) {
	            closesocket (ClientSocket);
	            WSACleanup ();
	            running.store (false);
	            return;
	         }
	      }

	      int httpver = (strstr (buffer, "HTTP/1.1") != NULL) ? 11 : 10;
	      if (httpver == 11) 
//	HTTP 1.1 defaults to keep-alive, unless close is specified. 
	         keepalive = strstr (buffer, "Connection: close") == NULL;
	      else // httpver == 10
	         keepalive = strstr (buffer, "Connection: keep-alive") != NULL;

	/* Identify the URL. */
	      char *p = strchr (buffer,' ');
	      if (p == NULL) 
	         break;
	      url = ++p; // Now this should point to the requested URL. 
	      p = strchr (p, ' ');
	      if (p == NULL)
	         break;
	      *p = '\0';

//	Select the content to send, we have just two so far:
//	 "/" -> Our google map application.
//	 "/data.json" -> Our ajax request to update planes. */
	   bool jsonUpdate	= false;
	      if (strstr (url, "/data.json")) {
	         content	= aircraftsToJson (parent -> planeList);
                 ctype		= "application/json;charset=utf-8";
	         jsonUpdate	= true;
	      }
	      else {
	         content	= theMap (mapFile);
	         ctype		= "text/html;charset=utf-8";
	      }

//	Create the header 
	      char hdr [2048];
	      sprintf (hdr,
	               "HTTP/1.1 200 OK\r\n"
	               "Server: SDRunoPlugin_1090\r\n"
	               "Content-Type: %s\r\n"
	               "Connection: %s\r\n"
	               "Content-Length: %d\r\n"
//	               "Access-Control-Allow-Origin: *\r\n"
	               "\r\n",
	               ctype. c_str (),
	               keepalive ? "keep-alive" : "close",
	               (int)(strlen (content. c_str ())));
	      int hdrlen = strlen (hdr);
//		  parent->show_text(std::string(hdr));
//	      if (jsonUpdate) {
//	         parent -> show_text (std::string ("Json update requested\n"));
//	         parent -> show_text (content);
//	      }
//	and send the reply
	      if ((send (ClientSocket, hdr, hdrlen, 0) == SOCKET_ERROR) ||
	          (send (ClientSocket, content. c_str (),
	                          content. size (), 0) == SOCKET_ERROR))  {
	         fprintf (stderr, "WRITE PROBLEM\n");
	         break;
	      }
	   }
	}
// cleanup
	closesocket(ClientSocket);
	WSACleanup();
	running. store (false);
}
 
std::string	httpHandler::theMap (std::string fileName) {
FILE	*fd;
std::string res;
int	bodySize;
char	*body;
	fd	=  fopen (fileName. c_str (), "r");
	fseek (fd, 0L, SEEK_END);
	bodySize	= ftell (fd);
	fseek (fd, 0L, SEEK_SET);
        body =  (char *)malloc (bodySize);
        fread (body, 1, bodySize, fd);
        fclose (fd);
	res	= std::string (body);
	free (body);
	return res;
}

