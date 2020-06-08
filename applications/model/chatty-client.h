/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Georgia Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: George F. Riley <riley@ece.gatech.edu>
 */

#ifndef CHATTY_CLIENT_H
#define CHATTY_CLIENT_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"


namespace ns3 {

class Address;
class Socket;
// define this class in a public header



/**
 * \ingroup applications
 * \defgroup bulksend ChattyClient
 *
 * This traffic generator simply sends data
 * as fast as possible up to MaxBytes or until
 * the application is stopped (if MaxBytes is
 * zero). Once the lower layer send buffer is
 * filled, it waits until space is free to
 * send more data, essentially keeping a
 * constant flow of data. Only SOCK_STREAM 
 * and SOCK_SEQPACKET sockets are supported. 
 * For example, TCP sockets can be used, but 
 * UDP sockets can not be used.
 */

/**
 * \ingroup bulksend
 *
 * \brief Send as much traffic as possible, trying to fill the bandwidth.
 *
 * This traffic generator simply sends data
 * as fast as possible up to MaxBytes or until
 * the application is stopped (if MaxBytes is
 * zero). Once the lower layer send buffer is
 * filled, it waits until space is free to
 * send more data, essentially keeping a
 * constant flow of data. Only SOCK_STREAM
 * and SOCK_SEQPACKET sockets are supported.
 * For example, TCP sockets can be used, but
 * UDP sockets can not be used.
 *
 */
class ChattyClient : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  ChattyClient ();

  virtual ~ChattyClient ();

  /**
   * \brief Set the upper bound for the total number of bytes to send.
   *
   * Once this bound is reached, no more application bytes are sent. If the
   * application is stopped during the simulation and restarted, the 
   * total number of bytes sent is not reset; however, the maxBytes 
   * bound is still effective and the application will continue sending 
   * up to maxBytes. The value zero for maxBytes means that 
   * there is no upper bound; i.e. data is sent until the application 
   * or simulation is stopped.
   *
   * \param maxBytes the upper bound of bytes to send
   */
  void SetMaxTurns (uint32_t maxTurns);

  /** Set the data fill of the packet (what is sent as data to the server) to
    * the zero-terminated contents of the fill string string.
    *
    * \warning The size of resulting echo packets will be automatically adjusted
    * to reflect the size of the fill string -- this means that the PacketSize
    * attribute may be changed as a result of this call.
    *
    * \param fill The string to use as the actual echo data bytes.
    */
   void SetFill (std::string fill);

  /**
   * \brief Get the socket this application is attached to.
   * \return pointer to associated socket
   */
  Ptr<Socket> GetSocket (void) const;

protected:
  virtual void DoDispose (void);
private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop
  void ScheduleTransmit (Time dt);
  void openConn (int i);
  /**
   * \brief Send data until the L4 transmission buffer is full.
   */
  void SendData (Ptr<Socket> socket);

  Ptr<Socket>     m_socket;       //!< Associated socket
  Ptr<Socket>     new_socket[10];       //!< Associated socket
  std::list<Ptr<Socket> > m_socketList; //!< the accepted sockets
  Ptr<Socket>     m_socket2;       //!< Associated socket
  Address         m_peer;         //!< Peer address
  bool            m_connected;    //!< True if connected
  uint32_t        m_sendSize;     //!< Size of data to send each time
  uint32_t        m_maxTurns;     //!< Limit total number of bytes sent
  uint32_t        m_totTurns;     //!< Total bytes sent so far
  TypeId          m_tid;          //!< The type of protocol to use.
  uint32_t        m_totalRx;
  uint32_t        m_totalRxCount;
  uint32_t        m_midRecvCnt;     //!< Limit total number of bytes sent
  uint32_t        m_2ndRecvCnt;     //!< Limit total number of bytes sent
  uint32_t        m_closeCnt;     //!< Limit total number of bytes sent
  uint8_t *m_data; //!< packet payload data
  uint32_t m_dataSize; //!< packet payload size (must be equal to m_size)
  Time m_interval; //!< Packet inter-send time
  EventId m_sendEvent1; //!< Event to send the next packet
  std::map < Ptr<Socket>, uint32_t> client_sendSize;
  std::map < Ptr<Socket>, uint32_t> client_recvCnt;
  std::map < Ptr<Socket>, uint32_t> client_sendCnt;
  /// Traced Callback: sent packets
  TracedCallback<Ptr<const Packet> > m_txTrace;
  TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;

private:
  /**
   * \brief Connection Succeeded (called by Socket through a callback)
   * \param socket the connected socket
   */
  void ConnectionSucceeded (Ptr<Socket> socket);
  /**
   * \brief Connection Failed (called by Socket through a callback)
   * \param socket the connected socket
   */
  void ConnectionFailed (Ptr<Socket> socket);
  /**
   * \brief Send more data as soon as some has been transmitted.
   */
  void DataSend (Ptr<Socket>, uint32_t); // for socket's SetSendCallback

  void DataRecv (Ptr<Socket>); // for socket's SetRecvCallback
  void DataRecv1 (Ptr<Socket>); // for socket's SetRecvCallback
  void DataRecvNewClient (Ptr<Socket>); // for socket's SetRecvCallback
  void DataRecvMiddleClient (Ptr<Socket>); // for socket's SetRecvCallback
  void close_socket(Ptr<Socket>);
};

} // namespace ns3

#endif /* BULK_SEND_APPLICATION_H */
