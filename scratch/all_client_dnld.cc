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



using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MyWireless");

int 
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nCsma = 2;
  uint32_t nWifi = 5;
  uint32_t maxBytes = 5242880;
  //double rss = -44;  // -dBm
  bool tracing = true;
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448));
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
   // turn off RTS/CTS for frames below 2200 bytes
   Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

  cmd.Parse (argc,argv);
  NodeContainer csmaNodes;
  csmaNodes.Create (2);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  //csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(350)));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode = csmaNodes.Get (0);
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
   // This is one parameter that matters when using FixedRssLossModel
     // set it to zero; otherwise, gain will be added
     //phy.Set ("RxGain", DoubleValue (0) );

  YansWifiChannelHelper channel=YansWifiChannelHelper::Default ();;
  //channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    // The below FixedRssLossModel will cause the rss to be fixed regardless
    // of the distance between the two stations, and the transmit power
    //channel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));
    phy.SetChannel (channel.Create ());

  WifiHelper wifi= WifiHelper::Default ();;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
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
        queue->SetMaxSize(50);
        std::cout<<"Queue size: " << queue->GetMaxSize()<<std::endl;
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);
 /*mob = wifiStaNodes.Get(5)->GetObject<MobilityModel>();
    pos = mob->GetPosition();
    std::cout<<"sta position: x="<<pos.x<<",y="<<pos.y<<std::endl;*/
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
        std::cout << "i= " << i << std::endl;
         BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (wifiInterfaces.GetAddress (i), port));
        // Set the amount of data to send in bytes.  Zero is unlimited.
        source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
         ApplicationContainer sourceApps = source.Install (csmaNodes.Get (nCsma-1));
        sourceApps.Start (Seconds (0.1));
        sourceApps.Stop (Seconds (500.0));

//
// Create a PacketSinkApplication and install it on node 1
//
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  sinkApps = sink.Install (wifiStaNodes.Get (i));
  sA.Add(sinkApps);
  sinkApps.Start (Seconds (0.1));
  sinkApps.Stop (Seconds (500.0));
  }
   if (tracing)
    {
      AsciiTraceHelper ascii;
      csma.EnablePcapAll ("analysis/traffic_model/11g_mod/only_dnld5c", false);

    }


  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  phy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  phy.EnablePcap ("analysis/traffic_model/11g_mod/ap_only_dnld5c", apDevices.Get (0));
  csma.EnablePcap ("third", csmaDevices.Get (1), true);
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (500.0));
  //FlowMonitorHelper flowmon;
   // Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Run ();

    // 10. Print per flow statistics
   /* monitor->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
      {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
        std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / 50.0 / 1024 / 1024  << " Mbps\n";
      }*/
  Simulator::Destroy ();
  Ptr<PacketSink> sink1;
  for (unsigned int i = 0; i < nWifi; i++){
  sink1 = DynamicCast<PacketSink> (sA.Get (i));
  std::cout << "Total Bytes Received By : " << i<<" = "<<sink1->GetTotalRx ()<<std::endl;
  }
  return 0;
}
