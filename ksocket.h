#ifndef _KSOCKET_H_
#define _KSOCKET_H_


/**
  * created by icephoton drswinghead@163.com 2005-8-5
  */

#include <iostream>

#include <sys/socket.h>

class KSocket{



public:

	//=
	// Constructor
	//=
	KSocket(int  pFamily = PF_INET, int pType = SOCK_STREAM , int pProtocal = 0);

	//=
	// Destructor
	//=
	~KSocket();



	//=
	// Sets the timeout value for Connect, Read and Send operations.
	// Setting the timeout to 0 removes the timeout - making the Socket blocking.
	//=
	void setTimeout(int seconds, int microseconds);

	//=
	// Returns a description of the last known error
	//=
	const char *getError();

	//=
	// Connects to the specified server and port
	// If proxies have been specified, the connection passes through tem first.
	//=
	bool Connect(const char *pServer, int pPort);

	//=
	// Connects to the specified server and port over a secure connection
	// If proxies have been specified, the connection passes through them first.
	//=
	bool ConnectSSL(const char *pServer, int pPort);

	//=
	// Inserts a transparent tunnel into the connect chain
	// A transparent Tunnel is a server that accepts a connection on a certain port,
	// and always connects to a particular server:port address on the other side.
	// Becomes the last server connected to in the chain before connecting to the destination server
	//=
	bool insertTunnel(const char *pServer, int pPort);

	//=
	// Inserts a Socks5 server into the connect chain
	// Becomes the last server connected to in the chain before connecting to the destination server
	// 
	//=
	bool insertSocks5(const char *server, int port, const char *username, const char *password);

	//=
	// Inserts a Socks4 server into the connect chain
	// Becomes the last server connected to in the chain before connecting to the destination server
	//=
	bool insertSocks4(const char *server, int port, const char *username);

	//=
	// Inserts a CONNECT-Enabled HTTP proxy into the connect chain
	// Becomes the last server connected to in the chain before connecting to the destination server
	//
	//=
	bool insertProxy(const char *server, int port);

	//=
	// Sends a buffer of data over the connection
	// A connection must established before this method can be called
	//=
	int Write(const char *pData, int pLength);

	//=
	// Sends a buffer of data over the connection
	// A connection must established before this method can be called
	//=
	int Writeline(const char *pData, int pLength);
	

	//=
	// Reads a buffer of data from the connection
	// A connection must established before this method can be called
	//=
	int Read(char *pBuffer, int pLength);

	//=
	// Reads everything available from the connection
	// A connection must established before this method can be called
	//=
	char *Reads(char * rbuf) ;

	int Peek(char * rbuf, int pLength) ;

	//=
	// Reads a line from the connection
	// A connection must established before this method can be called
	//=
	int Readline(char *pLine , int pLength );

	//=
	// Sends a null terminated string over the connection
	// The string can contain its own newline characters.
	// Returns false and sets the error message if it fails to send the line.
	// A connection must established before this method can be called
	// 
	//=
	bool sends(const char *buffer);

	//=
	// Closes the connection
	// A connection must established before this method can be called
	//=
	bool Close();

	//=
	// Sets an output stream to receive realtime messages about the socket
	//=
	void setMessageStream(std::ostream &o);

	//add for test aim
	int GetSocket();
	bool SetSocket(KSocket * sock) ;

private:

	bool init(int  pFamily = PF_INET, int pType = SOCK_STREAM , int pProtocal = 0);

	///socket Ã◊Ω”√Ë ˆ◊÷
	int mSockFD;

	///¥ÌŒÛ∫≈
	int mErrorNo ;
	///¥ÌŒÛ–≈œ¢√Ë ˆ
	int mErrorStr[128];

	///
	int mFamily;

	///
	int mType;
	///
	int mProtocol;

	///
	int mTimeOut ;
	int mTimeOutUS ;
	int mTORetry;

};

//int nanosleep(const timespec*, timespec*)
//unsigned int usleep(unsigned int)

#endif

