/*
 * NetworkResponder.h
 *
 *  Created on: 14 Apr 2017
 *      Author: David
 */

#ifndef SRC_SAME70_NETWORKRESPONDER_H_
#define SRC_SAME70_NETWORKRESPONDER_H_

#include "RepRapFirmware.h"
#include "RepRap.h"
#include "NetworkDefs.h"
#include "Storage/FileData.h"
#include "NetworkBuffer.h"

// Forward declarations
class NetworkResponder;
class Socket;

// Class to implement a simple lock
class NetworkResponderLock
{
public:
	NetworkResponderLock() : owner(nullptr) { }

	bool Acquire(const NetworkResponder *who);
	void Release(const NetworkResponder *who);
	bool IsOwnedBy(const NetworkResponder *who) const { return owner == who; }

private:
	const NetworkResponder *owner;
};

// Network responder base class
class NetworkResponder
{
public:
	NetworkResponder *GetNext() const { return next; }
	virtual bool Spin() = 0;							// do some work, returning true if we did anything significant
	virtual bool Accept(Socket *s, Protocol protocol) = 0;	// ask the responder to accept this connection, returns true if it did
	virtual void Terminate(Protocol protocol) = 0;		// terminate the responder if it is serving the specified protocol
	virtual void Diagnostics(MessageType mtype) const = 0;

protected:
	// States machine control. Not all derived classes use all states.
	enum class ResponderState
	{
		free = 0,										// ready to be allocated
		reading,										// ready to receive data
		sending,										// sending data
		uploading,										// uploading a file to SD card

		// HTTP responder additional states
		gettingFileInfoLock,							// waiting to get the file info lock
		gettingFileInfo,								// getting file info

		// FTP responder additional states
		waitingForPasvPort,
		pasvPortOpened,
		sendingPasvData,
		pasvTransferComplete,

		// Telnet responder additional states
		justConnected,
		authenticating
	};

	NetworkResponder(NetworkResponder *n);

	void Commit(ResponderState nextState = ResponderState::free);
	virtual void SendData();
	virtual void ConnectionLost();

	void StartUpload(FileStore *file, const char *fileName);
	void FinishUpload(uint32_t fileLength, time_t fileLastModified);
	virtual void CancelUpload();

	uint32_t GetRemoteIP() const;

	static Platform& GetPlatform() { return reprap.GetPlatform(); }
	static Network& GetNetwork() { return reprap.GetNetwork(); }

	// General state
	NetworkResponder *next;								// next responder in the list
	ResponderState responderState;						// the current state
	ResponderState stateAfterSending;					// if we are sending, the state to enter when sending is complete
	Socket *skt;
	uint32_t timer;										// a general purpose millisecond timer

	// Buffers for sending responses
	OutputBuffer *outBuf;
	OutputStack *outStack;
	FileStore *fileBeingSent;
	NetworkBuffer *fileBuffer;

	// File uploads
	FileData fileBeingUploaded;
	char filenameBeingUploaded[FILENAME_LENGTH];
	uint32_t postFileLength, uploadedBytes;				// How many POST bytes do we expect and how many have already been written?
	time_t fileLastModified;
	bool uploadError;
};

#endif /* SRC_SAME70_NETWORKRESPONDER_H_ */
