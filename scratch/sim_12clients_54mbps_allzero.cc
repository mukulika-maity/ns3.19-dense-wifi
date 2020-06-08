/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |  
//  n12--n2  n1    n0  n1   
//                 |   |   
//                =======
//                LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

int 
main (int argc, char *argv[])
{
  bool verbose = true;
//  uint32_t nCsma = 3;
  uint32_t nWifi = 12;

   Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (1460));
  CommandLine cmd;
/*
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
*/
  cmd.Parse (argc,argv);

  if (nWifi > 18)
    {
      std::cout << "Number of wifi nodes " << nWifi << 
                   " specified exceeds the mobility bounding box" << std::endl;
      exit (1);
    }

  if (verbose)
    {
      //LogComponentEnable ("BulkSendApplication", LOG_LEVEL_ALL);
      //LogComponentEnable ("PacketSinkApplication", LOG_LEVEL_INFO);
    }


  NodeContainer csmaNodes;
  csmaNodes.Create (2);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode = csmaNodes.Get (0);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("OfdmRate54Mbps"));

  NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();

  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

/*  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));
   
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
*/

   //Fix the position of all WiFi clients at (0,0,0)
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

  InternetStackHelper stack;
  stack.Install (csmaNodes);
//  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;


  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer staInterfaces = address.Assign (staDevices);
  address.Assign (apDevices);

/*  uint16_t port = 5000;
  uint32_t maxBytes= 9*1000*1000;
  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (staInterfaces.GetAddress(1), port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps = source.Install (csmaNodes.Get (1));
  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (Seconds (10.0));

  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (wifiStaNodes.Get (1));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (10.0));

*/

   uint16_t port = 5000;
   PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
   

   // Configure packet source and sink at all clients

     //float sourceStart[12] = {83.416, 87.127, 95.001, 82.040, 86.732, 95.92, 94.720, 92.42, 87.115, 101.563, 92.838, 113.909};
     float sourceStart[12] = {10,10,10,11,11,11,12,12,12,13,13,13};
     int sourceStartBase = 10;
     float sourceDelta = 0.33;
     for(int i=0;i<12;i++)
		sourceStart[i] = sourceStartBase + sourceDelta*i;
     
	 //request from clients to server
	 uint32_t reqBytes= 1*1024;
	 int totalRequests = 6;
	 float requestDiff = 0.5;
	 for(int repeat=0; repeat<totalRequests; repeat++)
	 for(int client=0; client<12; client++) {
		 BulkSendHelper source0 ("ns3::TcpSocketFactory",
							 InetSocketAddress (staInterfaces.GetAddress(client), port));
		 // Set the amount of data to send in bytes.  Zero is unlimited.
		 source0.SetAttribute ("MaxBytes", UintegerValue (reqBytes));
		 ApplicationContainer sourceApps0 = source0.Install (wifiStaNodes.Get (client));
		 sourceApps0.Start (Seconds (sourceStart[client]-requestDiff));
		 sourceApps0.Stop (Seconds (500.0));

		 ApplicationContainer sinkApps0 = sink.Install (csmaNodes.Get (1));
		 sinkApps0.Start (Seconds (0.0));
		 sinkApps0.Stop (Seconds (500.0));
	 }
	 
	 //response from server to clients
	 uint32_t respBytes= 1*1024;
	 float respDiff = 0.3;
	 for(int repeat=0; repeat<totalRequests-1; repeat++)
	 for(int client=0; client<12; client++) {
		 BulkSendHelper source0 ("ns3::TcpSocketFactory",
							 InetSocketAddress (staInterfaces.GetAddress(client), port));
		 // Set the amount of data to send in bytes.  Zero is unlimited.
		 source0.SetAttribute ("MaxBytes", UintegerValue (respBytes));
		 ApplicationContainer sourceApps0 = source0.Install (csmaNodes.Get (1));
		 sourceApps0.Start (Seconds (sourceStart[client]-respDiff));
		 sourceApps0.Stop (Seconds (500.0));

		 ApplicationContainer sinkApps0 = sink.Install (wifiStaNodes.Get (client));
		 sinkApps0.Start (Seconds (0.0));
		 sinkApps0.Stop (Seconds (500.0));
	 }
	 
	 //big file download
	 uint32_t maxBytes= 9*1024*1024;
	 for(int client=0; client<12; client++) {
		 BulkSendHelper source0 ("ns3::TcpSocketFactory",
							 InetSocketAddress (staInterfaces.GetAddress(client), port));
		 // Set the amount of data to send in bytes.  Zero is unlimited.
		 source0.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
		 ApplicationContainer sourceApps0 = source0.Install (csmaNodes.Get (1));
		 sourceApps0.Start (Seconds (sourceStart[client]));
		 sourceApps0.Stop (Seconds (500.0));

		 ApplicationContainer sinkApps0 = sink.Install (wifiStaNodes.Get (client));
		 sinkApps0.Start (Seconds (0.0));
		 sinkApps0.Stop (Seconds (500.0));
	 }
     
   Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

   Simulator::Stop (Seconds (500.0));

   phy.EnablePcap ("sim", apDevices.Get (0));
   csma.EnablePcap ("sim", csmaDevices.Get (1), true);

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
