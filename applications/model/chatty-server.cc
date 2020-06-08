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
#include "ns3/random-variable.h"
//#include "../../fd-net-device/helper/encode-decode.h"
#include "chatty-server.h"
#include "mytag.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ChattyServer")
  ;
NS_OBJECT_ENSURE_REGISTERED (ChattyServer)
  ;

TypeId
ChattyServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ChattyServer")
    .SetParent<Application> ()
    .AddConstructor<ChattyServer> ()
    .AddAttribute ("Local", "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&ChattyServer::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol", "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&ChattyServer::m_tid),
                   MakeTypeIdChecker ())
    .AddAttribute ("SendSize", "The amount of data to send each time.",
                    UintegerValue (600),
                    MakeUintegerAccessor (&ChattyServer::m_sendSize),
                    MakeUintegerChecker<uint32_t> (1))
     .AddAttribute ("MaxBytes",
                     "The total number of bytes to send. "
                     "Once these bytes are sent, "
                     "no data  is sent again. The value zero means "
                      "that there is no limit.",
                      UintegerValue (0),
                      MakeUintegerAccessor (&ChattyServer::m_maxBytes),
                      MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Rx", "A packet has been received",
                     MakeTraceSourceAccessor (&ChattyServer::m_rxTrace))
  ;
  return tid;
}

ChattyServer::ChattyServer ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_totalRx = 0;
  m_totBytes=0;
  //client_connected=false;
}

ChattyServer::~ChattyServer()
{
  NS_LOG_FUNCTION (this);
}

uint32_t ChattyServer::GetTotalRx () const
{
  NS_LOG_FUNCTION (this);
  return m_totalRx;
}

Ptr<Socket>
ChattyServer::GetListeningSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

std::list<Ptr<Socket> >
ChattyServer::GetAcceptedSockets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketList;
}

void ChattyServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socketList.clear ();

  // chain up
  Application::DoDispose ();
}


// Application Methods
void ChattyServer::StartApplication ()    // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  // Create the socket if not already
  if (!m_socket)
    {
	  std::cout<<"type id="<<m_tid<<std::endl;
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      m_socket->Bind (m_local);
      m_socket->Listen ();
      //m_socket->ShutdownSend ();
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

  m_socket->SetRecvCallback (MakeCallback (&ChattyServer::HandleRead, this));
  //m_socket->SetSendCallback(MakeCallback (&ChattyServer::DataSend, this));
  m_socket->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&ChattyServer::HandleAccept, this));
  m_socket->SetCloseCallbacks (
    MakeCallback (&ChattyServer::HandlePeerClose, this),
    MakeCallback (&ChattyServer::HandlePeerError, this));
}

void ChattyServer::StopApplication ()     // Called at time specified by Stop
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

void ChattyServer::HandleRead (Ptr<Socket> socket)
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
      int currSize=packet->GetSize();
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
      if(packet->GetSize()==685){
    	  m_from=from;
    	  //m_connected=true;
    	  //int value=client_list[socket];
    	  //std::cout<<"last packet received value="<<value<<std::endl;
    	  m_sendSize=1448;
    	  client_connected[socket]=true;
    	 SendData(socket);
    	 //SendData();
      }
      else{
    	  if(currSize==311){
    		  m_sendSize=749;//main page w/0 auth
    	  }
    	  if(currSize==574){
    		  m_sendSize=929;//main page with auth
    	  }
    	  if(currSize==327){
    		  m_sendSize=437;//blank.gif
    	  }
    	  else if(currSize==320){
    		  m_sendSize=506;//back.gif
    	  }
    	  else if(currSize==332){
    		  m_sendSize=1330;//compressed.gif
    	  }
    	  else if(currSize==325){
    		  m_sendSize=533;//movie.gif
    	  }
    	  else if(currSize==328){
    		  m_sendSize=519;//text.gif
    	  }
    	  else if(currSize==335){
    		  m_sendSize=535;//unknown.gif
    	  }
    	  else if(currSize==269){
    		  m_sendSize=502;//favicon.ico
    	  }
    	  else if(currSize==299){
    		  m_sendSize=535;//favicon.ico twice
    	  }


        // Make sure we don't send too many
        UniformVariable var;
        double rand = var.GetValue(0.00034,0.00055);
        Time dt=Seconds(rand);
        std::cout<<"time now="<<Simulator::Now().GetSeconds()<<"rand="<<rand<<"var in server="<<dt.GetSeconds()<<std::endl;
        m_sendEvent = Simulator::Schedule (dt, &ChattyServer::Send, this,socket,m_sendSize);

    }
    }
}


void ChattyServer::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void ChattyServer::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
/*static void
CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "cwnd\t" <<newCwnd);
}*/

void ChattyServer::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
 // s->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));
  s->SetRecvCallback (MakeCallback (&ChattyServer::HandleRead,this));
  s->SetSendCallback(MakeCallback (&ChattyServer::DataSend, this));
  client_list[s]=0;

 /* m_client.m_socket=s;
  m_client.totBytes=0
  clients.push_back(m_client);*/
  m_socketList.push_back (s);
}
void ChattyServer::DataSend (Ptr<Socket> socket, uint32_t)
{
  NS_LOG_FUNCTION (this);

  if (client_connected[socket]){
    //{ // Only send new data if the connection has completed
      Simulator::ScheduleNow (&ChattyServer::SendData, this,socket);
    }
}
// Private helpers

void ChattyServer::SendData (Ptr<Socket> socket)
//void ChattyServer::SendData (void)
{
  NS_LOG_FUNCTION (this);
 // std::cout<<"in send data address="<<InetSocketAddress::ConvertFrom(from).GetIpv4()<<std::endl;

  while (m_maxBytes == 0 || client_list[socket] < m_maxBytes)
    { // Time to send more
      uint32_t toSend = m_sendSize;
      // Make sure we don't send too many
      if (m_maxBytes > 0)
        {
          toSend = std::min (m_sendSize, m_maxBytes - client_list[socket]);
        }
      NS_LOG_LOGIC ("sending packet at " << Simulator::Now ());
      Ptr<Packet> packet = Create<Packet> (toSend);
      m_txTrace (packet);
      int actual = socket->Send(packet);
      //int actual;
      if (actual > 0)
        {
    	  client_list[socket] += actual;
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
  if (client_list[socket] == m_maxBytes && client_connected[socket])
    {
      socket->Close ();
      client_connected[socket] = false;
    }
}
void ChattyServer::Send(Ptr<Socket> socket, int size)
//void ChattyServer::SendData (void)
{
	 uint32_t toSend = size;
	NS_LOG_LOGIC ("sending packet at " << Simulator::Now ());
	Ptr<Packet> sendPacket = Create<Packet> (toSend);
	NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
			<< "s chatty server packet send to ");
	m_txTrace (sendPacket);
	int actual = socket->Send(sendPacket);
	// We exit this loop when actual < toSend as the send side
	// buffer is full. The "DataSent" callback will pop when
	// some buffer space has freed ip.
	if ((unsigned)actual < toSend)
	{
		NS_LOG_ERROR("Message not sent");
	}
	NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
			<< "s chatty server sent "<<actual<<"Bytes");
}


} // Namespace ns3
