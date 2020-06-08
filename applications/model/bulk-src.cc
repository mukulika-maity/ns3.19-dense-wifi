/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/flow-id-tag.h"
//#include "../../fd-net-device/helper/encode-decode.h"
#include "bulk-src.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BulkServer")
  ;
NS_OBJECT_ENSURE_REGISTERED (BulkServer)
  ;

TypeId
BulkServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BulkServer")
    .SetParent<Application> ()
    .AddConstructor<BulkServer> ()
    .AddAttribute ("Local", "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&BulkServer::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol", "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&BulkServer::m_tid),
                   MakeTypeIdChecker ())
    .AddAttribute ("SendSize", "The amount of data to send each time.",
                    UintegerValue (1024),
                    MakeUintegerAccessor (&BulkServer::m_sendSize),
                    MakeUintegerChecker<uint32_t> (1))
	.AddAttribute ("MaxBytes",
				"The total number of bytes to send. "
				"Once these bytes are sent, "
				"no data  is sent again. The value zero means "
				"that there is no limit.",
				UintegerValue (0),
				MakeUintegerAccessor (&BulkServer::m_maxBytes),
				MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Rx", "A packet has been received",
                     MakeTraceSourceAccessor (&BulkServer::m_rxTrace))
  ;
  return tid;
}

BulkServer::BulkServer ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_totalRx = 0;
}

BulkServer::~BulkServer()
{
  NS_LOG_FUNCTION (this);
}

uint32_t BulkServer::GetTotalRx () const
{
  NS_LOG_FUNCTION (this);
  return m_totalRx;
}

Ptr<Socket>
BulkServer::GetListeningSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

std::list<Ptr<Socket> >
BulkServer::GetAcceptedSockets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketList;
}

void BulkServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socketList.clear ();

  // chain up
  Application::DoDispose ();
}


// Application Methods
void BulkServer::StartApplication ()    // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      m_socket->Bind (m_local);
      m_socket->Listen ();
      //m_socket->ShutdownRecv();
      if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
            }
        }
    }


  m_socket->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&BulkServer::HandleAccept, this));
  m_socket->SetCloseCallbacks (
    MakeCallback (&BulkServer::HandlePeerClose, this),
    MakeCallback (&BulkServer::HandlePeerError, this));

}

void BulkServer::StopApplication ()     // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  while(!m_socketList.empty ()) //these are accepted sockets, close them
    {
      Ptr<Socket> acceptedSocket = m_socketList.front ();
      m_socketList.pop_front ();
      acceptedSocket->Close ();
    }
  if (m_socket)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}



void BulkServer::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void BulkServer::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}


void BulkServer::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetSendCallback(MakeCallback (&BulkServer::DataSend, this));
  s->SetRecvCallback (MakeCallback (&BulkServer::HandleRead, this));
  m_socketList.push_back (s);
  m_connected=true;
  //SendData();
}
void BulkServer::DataSend (Ptr<Socket>, uint32_t)
{
  NS_LOG_FUNCTION (this);

  if (m_connected)
    { // Only send new data if the connection has completed
      Simulator::ScheduleNow (&BulkServer::HandleRead, this);
    }
}
// Private helpers

/*void BulkServer::SendData (Ptr<Socket> socket, Address from)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO("in send");
  while (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    { // Time to send more
      uint32_t toSend = m_sendSize;
      // Make sure we don't send too many
      if (m_maxBytes > 0)
        {
          toSend = std::min (m_sendSize, m_maxBytes - m_totBytes);
        }
      NS_LOG_LOGIC ("sending packet at " << Simulator::Now ());
      Ptr<Packet> packet = Create<Packet> (toSend);
      m_txTrace (packet);
      int actual = socket->SendTo (packet,0,from);
      if (actual > 0)
        {
          m_totBytes += actual;
        }
      // We exit this loop when actual < toSend as the send side
      // buffer is full. The "DataSent" callback will pop when
      // some buffer space has freed ip.
      if ((unsigned)actual != toSend)
        {
          break;
        }
    }
  // Check if time to close (all sent)
  if (m_totBytes == m_maxBytes && m_connected)
    {
      m_socket->Close ();
      m_connected = false;
    }
}*/
void BulkServer::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
      m_totalRx += packet->GetSize ();

      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s chatty server received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes"<<"addr="<<m_local);
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s chatty server received "
                       <<  packet->GetSize () << " bytes from "
                       << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                       << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      m_rxTrace (packet, from);
    // SendData(socket,from);
    }
  NS_LOG_FUNCTION (this);
    NS_LOG_INFO("in send");
    while (m_maxBytes == 0 || m_totBytes < m_maxBytes)
      { // Time to send more
        uint32_t toSend = m_sendSize;
        // Make sure we don't send too many
        if (m_maxBytes > 0)
          {
            toSend = std::min (m_sendSize, m_maxBytes - m_totBytes);
          }
        NS_LOG_LOGIC ("sending packet at " << Simulator::Now ());
        Ptr<Packet> packet = Create<Packet> (toSend);
        m_txTrace (packet);
        int actual = socket->SendTo (packet,0,from);
        if (actual > 0)
          {
            m_totBytes += actual;
          }
        // We exit this loop when actual < toSend as the send side
        // buffer is full. The "DataSent" callback will pop when
        // some buffer space has freed ip.
        if ((unsigned)actual != toSend)
          {
            break;
          }
      }
    // Check if time to close (all sent)
    if (m_totBytes == m_maxBytes && m_connected)
      {
        socket->Close ();
        m_connected = false;
      }
}




} // Namespace ns3
