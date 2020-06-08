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
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MyWireless");
double old_time = 0.0;
EventId output,output1;
Time current = Time::FromInteger(3, Time::S);  //Only record cwnd and ssthresh values every 3 seconds
Time currentd = Time::FromDouble(0.01, Time::S);  //Only record cwnd and ssthresh values every 3 seconds
bool first = true;

/*void print(void){
	std::cout<<"Current Queue size: " <<std::endl;
	EventId m_Event = Simulator::Schedule (Seconds(2), &print, this);
}*/
static void
OutputTrace ()
{
 // *stream->GetStream() << newtime << " " << newval << std::endl;
 // old_time = newval;
}
static void
CwndTracer (Ptr<OutputStreamWrapper>stream, uint32_t oldval, uint32_t newval)
{
  double new_time = Simulator::Now().GetSeconds();
  if (old_time == 0 && first)
  {
    double mycurrent = current.GetSeconds();
    std::cout<<"cwnd "<< new_time << " " << mycurrent << " " << newval << std::endl;
    *stream->GetStream() << new_time << " " << mycurrent << " " << newval << std::endl;
    first = false;
    output = Simulator::Schedule(current,&OutputTrace);
  }
  else
  {
    if (output.IsExpired())
    {
    	 std::cout<<"cwnd "<< new_time << " " << newval << std::endl;
      *stream->GetStream() << new_time << " " << newval << std::endl;
      output.Cancel();
      output = Simulator::Schedule(current,&OutputTrace);
    }
  }
}
static void
BuffTracer (Ptr<OutputStreamWrapper>stream,NetDeviceContainer apDevices)
{
  double new_time = Simulator::Now().GetSeconds();
  if (old_time == 0 && first)
  {

    //double mycurrent = current.GetSeconds();
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

          std::cout<<"buff "<< new_time << " " << queue->GetSize()<< std::endl;
          *stream->GetStream() << new_time << " " << queue->GetSize() << std::endl;
          first = false;
          output1 = Simulator::Schedule(currentd,&BuffTracer,stream,apDevices);
  }
  else
  {
    if (output1.IsExpired())
    {
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
    	          std::cout<<"buff "<< new_time << " " << queue->GetSize() << std::endl;
    	          *stream->GetStream() << new_time << " " << queue->GetSize() << std::endl;
    	          output1.Cancel();
    	          output1 = Simulator::Schedule(currentd,&BuffTracer,stream,apDevices);
    }
  }
}

static void
SsThreshTracer (Ptr<OutputStreamWrapper>stream, uint32_t oldval, uint32_t newval)
{
  double new_time = Simulator::Now().GetSeconds();
  if (old_time == 0 && first)
  {
    double mycurrent = current.GetSeconds();
    *stream->GetStream() << new_time << " " << mycurrent << " " << newval << std::endl;
    first = false;
    output = Simulator::Schedule(current,&OutputTrace);
  }
  else
  {
    if (output.IsExpired())
    {
      *stream->GetStream() << new_time << " " << newval << std::endl;
      output.Cancel();
      output = Simulator::Schedule(current,&OutputTrace);
    }
  }
}

static void
TraceCwnd (std::string cwnd_tr_file_name)
{
  AsciiTraceHelper ascii;
  if (cwnd_tr_file_name.compare("") == 0)
     {
       NS_LOG_DEBUG ("No trace file for cwnd provided");
       return;
     }
  else
    {
      Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(cwnd_tr_file_name.c_str());
      std::cout<<"conwnd "<<std::endl;
      Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/1/CongestionWindow",MakeBoundCallback (&CwndTracer, stream));
    }
}

static void
TraceSsThresh(std::string ssthresh_tr_file_name)
{
  AsciiTraceHelper ascii;
  if (ssthresh_tr_file_name.compare("") == 0)
    {
      NS_LOG_DEBUG ("No trace file for ssthresh provided");
      return;
    }
  else
    {
      Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(ssthresh_tr_file_name.c_str());
      Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/SlowStartThreshold",MakeBoundCallback (&SsThreshTracer, stream));
    }
}

/*void SetCwMin(uint16_t cw)
{
   Config::Set("/NodeList/0/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/DcaTxop/CwMin", UintegerValue(cw));
}*/
int
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nCsma = 2;
  uint32_t nWifi = 30;
  uint32_t maxBytes = 9971233;
  uint32_t maxTurns = 100;
  std::string cwnd_tr_file_name = "cwnd.tr";
  std::string ssthresh_tr_file_name = "ssthresh.tr";
  //std::string buff_tr_file_name = "buff.tr";
  bool tracing = true;
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448));
  Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId()));
  //Config::SetDefault("ns3::TcpSocket::DelAckTimeout",TimeValue(Seconds(0.00005)));
  //Config::SetDefault("ns3::TcpSocket::DelAckTimeout",TimeValue(Seconds(0.00005)));
  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

  cmd.Parse (argc,argv);

  if (nWifi > 50)
    {
      std::cout << "Number of wifi nodes " << nWifi <<
                   " specified exceeds the mobility bounding box" << std::endl;
      exit (1);
    }

  if (verbose)
    {
      //LogComponentEnable ("DcfManager", LOG_LEVEL_DEBUG);
      //LogComponentEnable ("YansWifiPhy", LOG_LEVEL_DEBUG);
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

  WifiHelper wifi= WifiHelper::Default ();

  wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
  wifi.SetRemoteStationManager ("ns3::MinstrelWifiManager");
  //wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
    //                                "DataMode", StringValue ("OfdmRate54Mbps"));
 // Config::SetDefault ("$ns3::WifiNetDevice::RemoteStationManager", "ns3::RraaWifiManager");
  NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();

  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  //Ptr<WifiMacQueue> m_queue= wifi.
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
      std::cout<<"Current Queue size: " << queue->GetSize()<<std::endl;
      //Time now=Simulator::Now().GetSeconds();

      //EventId m_Event = Simulator::Schedule (now, &print, this);
  MobilityHelper mobility;



  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);

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
  //uint16_t port = 1044;  // well-known echo port number
    //    ApplicationContainer sinkApps,sA;
  std::cout << "Maxbytes " << maxBytes << std::endl;

  uint16_t servPort = 50;
  ChattyServerHelper chattyServer("ns3::TcpSocketFactory",
          InetSocketAddress (Ipv4Address::GetAny (), servPort));
  chattyServer.SetAttribute ("MaxBytes", UintegerValue (maxBytes));

    ApplicationContainer chattyServerApps = chattyServer.Install (csmaNodes.Get (nCsma-1));
    chattyServerApps.Start (Seconds (0.1));
    chattyServerApps.Stop (Seconds (500.0));
    std::cout<<"client id="<<wifiStaNodes.Get (1)->GetId()<<std::endl;
  for (unsigned int i = 0; i < nWifi;i++)
    {
      Address remoteAddress (InetSocketAddress (csmaInterfaces.GetAddress (nCsma-1), servPort));
      ChattyClientHelper chattyClient ("ns3::TcpSocketFactory",remoteAddress);
             // Set the amount of data to send in bytes.  Zero is unlimited.
      chattyClient.SetAttribute ("MaxTurns", UintegerValue (maxTurns));
              ApplicationContainer chattyClientApps = chattyClient.Install (wifiStaNodes.Get (i));
              int seed = i+1;
              SeedManager::SetSeed (seed);
              SeedManager::SetRun (1);
              UniformVariable var;
              double rand = var.GetValue(0.1,4);
              std::cout<<"var="<<rand<<std::endl;
              chattyClientApps.Start (Seconds (rand));
              chattyClientApps.Stop (Seconds (500.0));
    }
  uint16_t onoffPort = 4000;
     PacketSinkHelper onoffSink ("ns3::UdpSocketFactory",
                           InetSocketAddress (Ipv4Address::GetAny (), onoffPort));
      ApplicationContainer onoffsinkApps = onoffSink.Install (csmaNodes.Get (nCsma-1));
          onoffsinkApps.Start (Seconds (0.1));
          onoffsinkApps.Stop (Seconds (500.0));
    for (unsigned int i = 0; i < nWifi;i++)
      {
        Address remoteAddress (InetSocketAddress (csmaInterfaces.GetAddress (nCsma-1), onoffPort));
        OnOffHelper clientHelper ("ns3::UdpSocketFactory", remoteAddress);
        clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        clientHelper.SetAttribute ("PacketSize", StringValue ("300"));
        clientHelper.SetAttribute ("DataRate", StringValue ("26Kbps"));
        ApplicationContainer clientApp = clientHelper.Install (wifiStaNodes.Get (i));
        clientApp.Start (Seconds (0.1));
        clientApp.Stop (Seconds (500.0));
      }
   if (tracing)
    {
      AsciiTraceHelper ascii;
      csma.EnableAsciiAll (ascii.CreateFileStream ("analysis/traffic_model/chatty.tr"));
       csma.EnablePcapAll ("analysis/traffic_model/chatty_onoff", false);
       Simulator::Schedule(Seconds(0.00001), &TraceCwnd, cwnd_tr_file_name);
         Simulator::Schedule(Seconds(0.00001), &TraceSsThresh, ssthresh_tr_file_name);

        	 Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream("analysis/traffic_model/chatty_buff_onoff.tr");
         Simulator::Schedule(Seconds(0.00002), &BuffTracer, stream,apDevices);
    }
   //Simulator::Schedule(Seconds(0.00001), &SetCwMin, 32);
   //AthstatsHelper athstats;
     //athstats.EnableAthstats ("athstats-ap", wifiApNode);
  /*UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma-1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma-1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps =
    echoClient.Install (wifiStaNodes.Get (nWifi - 1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));*/

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  AsciiTraceHelper asci;
  phy.EnablePcap ("analysis/traffic_model/ap_logs_onoff", apDevices.Get (0));
  phy.EnableAscii(asci.CreateFileStream("analysis/wireless.tr"),apDevices.Get(0));
  csma.EnablePcap ("third", csmaDevices.Get (1), true);
  NS_LOG_INFO ("Run Simulation.");

  Simulator::Stop (Seconds (500.0));
  Simulator::Run ();
  Simulator::Destroy ();
  /*Ptr<PacketSink> sink1;
  for (unsigned int i = 0; i < nWifi; i++){
  sink1 = DynamicCast<PacketSink> (sA.Get (i));
  std::cout << "Total Bytes Received By : " << i<<" = "<<sink1->GetTotalRx ()<<std::endl;
  }*/
  return 0;
}
