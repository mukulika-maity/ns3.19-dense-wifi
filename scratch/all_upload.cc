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
#include "ns3/flow-monitor-module.h"


// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1 

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MyWireless");
double old_time = 0.0;
EventId output1;

Time currentd = Time::FromDouble(0.01, Time::S);  //Only record cwnd and ssthresh values every 3 seconds
bool first = true;


int 
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nCsma = 2;
  uint32_t nWifi = 30;//14;
  uint32_t maxBytes = 5242880;//9971233;
  //double rss = -44;  // -dBm
  bool tracing = true;
  std::string buff_tr_file_name = "bulk_buff.tr";
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448));
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
   // turn off RTS/CTS for frames below 2200 bytes
   Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));

  if (nWifi > 50)
    {
      std::cout << "Number of wifi nodes " << nWifi << 
                   " specified exceeds the mobility bounding box" << std::endl;
      exit (1);
    }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }


  NodeContainer csmaNodes;
  csmaNodes.Create (2);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  //csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(500)));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode = csmaNodes.Get (0);
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();

  YansWifiChannelHelper channel=YansWifiChannelHelper::Default ();;
  phy.SetChannel (channel.Create ());

  WifiHelper wifi= WifiHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
  //wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
  wifi.SetRemoteStationManager ("ns3::MinstrelWifiManager");
//wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
  //                                "DataMode", StringValue ("OfdmRate54Mbps"));
 // wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue ("OfdmRate54Mbps"),"ControlMode",StringValue ("OfdmRate54Mbps"));
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
 Ptr<WifiNetDevice> dev = DynamicCast<WifiNetDevice> (apDevices.Get (0));
        NS_ASSERT (dev != NULL);
        PointerValue ptr;
        dev->GetAttribute ("Mac",ptr);
        Ptr<RegularWifiMac> regmac = ptr.Get<RegularWifiMac> ();
        NS_ASSERT (regmac != NULL);
        regmac->GetAttribute ("DcaTxop",ptr);
        Ptr<DcaTxop> dca = ptr.Get<DcaTxop> ();
        NS_ASSERT (dca != NULL);
        Ptr<WifiMacQueue> queue = dca->GetQueue ();
        NS_ASSERT (queue != NULL);
        std::cout<<"Queue size: " << queue->GetMaxSize()<<std::endl;
        queue->SetMaxSize(512);
        std::cout<<"Queue size: " << queue->GetMaxSize()<<std::endl;
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (0.0),
                                   "MinY", DoubleValue (0.0),
                                   "DeltaX", DoubleValue (1.0),
                                   "DeltaY", DoubleValue (1.0),
                                   "GridWidth", UintegerValue (3),
                                   "LayoutType", StringValue ("RowFirst"));
  

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (1.5, 1.5, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);
  InternetStackHelper stack;
  stack.Install (csmaNodes);
  //stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;


  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer wifiInterfaces;
  wifiInterfaces=address.Assign (staDevices);
  address.Assign (apDevices);
  uint16_t port = 9;  // well-known echo port number
        ApplicationContainer sinkApps,sA;
  std::cout << "Maxbytes " << maxBytes << std::endl;
  for (unsigned int i = 0; i < nWifi; i++){
	  SeedManager::SetSeed (1);
	  	        		SeedManager::SetRun (1);
	  	        		UniformVariable var;
	  	        		int rand = var.GetValue(0,10);
	  	        		std::cout<<"var="<<rand<<std::endl;
        //std::cout << "i= " << i << std::endl;
         BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (csmaInterfaces.GetAddress (nCsma-1), port+i));
        // Set the amount of data to send in bytes.  Zero is unlimited.
        source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
         ApplicationContainer sourceApps = source.Install (wifiStaNodes.Get (i));
        sourceApps.Start (Seconds (0.1));
        sourceApps.Stop (Seconds (500.0));

//
// Create a PacketSinkApplication and install it on node 1
//
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port+i));
  sinkApps = sink.Install (csmaNodes.Get (nCsma-1));
  sA.Add(sinkApps);
  sinkApps.Start (Seconds (0.1));
  sinkApps.Stop (Seconds (500.0));
  }
   if (tracing)
    {
      AsciiTraceHelper ascii;
      //csma.EnableAsciiAll (ascii.CreateFileStream ("analysis/traffic_model/only_dnld_512.tr"));
      //csma.EnablePcapAll ("analysis/traffic_model/11g_mod/all_uplod_1s_5", false);
      // Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(buff_tr_file_name.c_str());
               //Simulator::Schedule(Seconds(0.00002), &BuffTracer, stream,apDevices);
    }


  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  phy.EnablePcap ("analysis/traffic_model/11g_mod/all_upload", staDevices.Get(5));
  phy.EnablePcap ("analysis/traffic_model/11g_mod/all_upload", staDevices.Get (20));
  phy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  	phy.EnablePcap ("analysis/traffic_model/11g_mod/ap_all_upload", apDevices.Get (0));

  csma.EnablePcap ("third", csmaDevices.Get (1), true);
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (500.0));
    Simulator::Run ();
  Ptr<PacketSink> sink1;
  for (unsigned int i = 0; i < nWifi; i++){
  sink1 = DynamicCast<PacketSink> (sA.Get (i));
  std::cout << "Total Bytes Received By : " << i<<" = "<<sink1->GetTotalRx ()<<std::endl;
  }
  return 0;
}
