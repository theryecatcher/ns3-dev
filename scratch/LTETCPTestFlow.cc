/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 */

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
//#include "ns3/gtk-config-store.h"

//Anoop
// #include <string>
#include <fstream>
#include "ns3/point-to-point-module.h"
#include "ns3/packet-sink.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE ("LTETCPTestFlow");

//TRACE SINK
double firstRxTime = Simulator::Now().GetSeconds(), lastRxTime;
uint32_t bytesTotal = 0;
AsciiTraceHelper thrAscii;
Ptr<OutputStreamWrapper> stream = thrAscii.CreateFileStream ("lte-tcp-flow.tr");

///*
void ThroughputMonitor (FlowMonitorHelper* fmhelper, Ptr<FlowMonitor> flowMon)
{
  flowMon->CheckForLostPackets(); 

  Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
  std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
  { 
    Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
    std::cout<<"Flow                  : " << fiveTuple.sourceAddress <<" -> "<< fiveTuple.destinationAddress<<std::endl;
    std::cout<<"Duration              : " << stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstRxPacket.GetSeconds()<<std::endl;
    std::cout<<"Last Received Packet  : " << stats->second.timeLastRxPacket.GetSeconds()<<" Seconds"<<std::endl;
    std::cout<<"Some stats            : " << stats->second.timeFirstRxPacket.GetSeconds()<<" Seconds"<<std::endl;
    std::cout<<"Throughput            : " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstRxPacket.GetSeconds())  << " bps"<<std::endl;
    std::cout<<"---------------------------------------------------------------------------"<<std::endl;
    if (fiveTuple.destinationAddress == "7.0.0.2")
      *stream->GetStream() << stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstRxPacket.GetSeconds() << "," << stats->second.rxBytes << "," << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstRxPacket.GetSeconds()) << std::endl;
  } 
  Simulator::Schedule(Seconds(1), &ThroughputMonitor, fmhelper, flowMon);
}
//*/
/*
void SinkRxTrace(Ptr<const Packet> pkt, const Address &addr)
{
  if (firstRxTime < 0)
    firstRxTime = Simulator::Now().GetSeconds();

  lastRxTime = Simulator::Now().GetSeconds();
  bytesTotal += pkt->GetSize();

  std::cout<<"Duration    : " << lastRxTime-firstRxTime <<std::endl;
  std::cout<<"Bytes       : " << bytesTotal <<std::endl;
  std::cout<<"Packet Size : " << pkt->GetSize() <<std::endl;
  std::cout<<"Throughput  : " << bytesTotal * 8.0 / (lastRxTime - firstRxTime) <<std::endl;

  *stream->GetStream() << Simulator::Now().GetSeconds() << "," << bytesTotal << "," << bytesTotal * 8.0 / (lastRxTime - firstRxTime) << std::endl;
  // 
}
*/
int main (int argc, char *argv[])
{ 

  uint16_t numberOfNodes = 1;
  double simTime = 120.0;
  // Assumption distance between the nodes
  double distance = 60.0;
  // double interPacketInterval = 100;

  // Anoop (referenced from tcp-bulk-send.cc)
  // Send 20 MB of data
  uint32_t maxBytes = 20000000;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("distance", "Distance between eNBs [m]", distance);
  // cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.Parse(argc, argv);

  Ptr<LteHelper> lteH = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteH->SetEpcHelper (epcHelper);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

   // Create a single server
  NodeContainer serverContainer;
  serverContainer.Create (1);
  Ptr<Node> server = serverContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (serverContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("10Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  // Install the internet no the Server/server
  NetDeviceContainer internetDevices = p2ph.Install (pgw, server);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address serverAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> serverStaticRouting = ipv4RoutingHelper.GetStaticRouting (server->GetObject<Ipv4> ());
  serverStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(numberOfNodes);
  ueNodes.Create(numberOfNodes);

  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfNodes; i++)
  {
    positionAlloc->Add (Vector(distance * i, 0, 0));
  }
  MobilityHelper mobility;
  // Assumption that we are using a constant distance model
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(enbNodes);
  mobility.Install(ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteH->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteH->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
  {
    Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
    Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
    ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  }

  // Attach one UE per eNodeB
  for (uint16_t i = 0; i < numberOfNodes; i++)
  {
    lteH->Attach (ueLteDevs.Get(i), enbLteDevs.Get(i));
        // side effect: the default EPS bearer will be activated
  }

/*
   // Install and start applications on UEs and remote host
   uint16_t dlPort = 1234;
   uint16_t ulPort = 2000;
   uint16_t otherPort = 3000;
   ApplicationContainer clientApps;
   ApplicationContainer serverApps;
   for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
     {
       ++ulPort;
       ++otherPort;
       PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
       PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
       PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));
       serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
       serverApps.Add (ulPacketSinkHelper.Install (server));
       serverApps.Add (packetSinkHelper.Install (ueNodes.Get(u)));

       UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
       dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
       dlClient.SetAttribute ("MaxPackets", UintegerValue(1000000));

       UdpClientHelper ulClient (serverAddr, ulPort);
       ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
       ulClient.SetAttribute ("MaxPackets", UintegerValue(1000000));

       UdpClientHelper client (ueIpIface.GetAddress (u), otherPort);
       client.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
       client.SetAttribute ("MaxPackets", UintegerValue(1000000));

       clientApps.Add (dlClient.Install (server));
       clientApps.Add (ulClient.Install (ueNodes.Get(u)));
       if (u+1 < ueNodes.GetN ())
         {
           clientApps.Add (client.Install (ueNodes.Get(u+1)));
         }
       else
         {
           clientApps.Add (client.Install (ueNodes.Get(0)));
         }
     }
   serverApps.Start (Seconds (0.01));
   clientApps.Start (Seconds (0.01));
   lteH->EnableTraces ();
   // Uncomment to enable PCAP tracing
   //p2ph.EnablePcapAll("lena-epc-first");
*/

  //Anoop (from tcp-bulk-send.cc)
  // Create a BulkSendApplication and install it on remote host
  uint16_t port = 9;  // well-known echo port number
  BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address ("7.0.0.2"), port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  // Set the segment size
  source.SetAttribute ("SendSize", UintegerValue (10000));
  ApplicationContainer sourceApps = source.Install (server);
  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (Seconds (simTime));

  // Create a PacketSinkApplication and install it on ueNode
  PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (ueNodes.Get (0));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (simTime));

  // Set up tracing
  AsciiTraceHelper ascii;
  p2ph.EnableAsciiAll (ascii.CreateFileStream ("lte-tcp-test-flow.tr"));
  p2ph.EnablePcapAll ("lte-tcp-test-flow", false);

  ///*
  // Throughput Monitor using FlowMon
  FlowMonitorHelper fmH;
  NodeContainer all_nodes;
  all_nodes.Add(ueNodes);
  all_nodes.Add(serverContainer);

  Ptr<FlowMonitor> monAll = fmH.Install(all_nodes);

  monAll->SetAttribute("DelayBinWidth", DoubleValue(0.001));
  //*/

  Simulator::Stop(Seconds(simTime + 0.001));

  //Connect trace source with trace sink
  // Config::ConnectWithoutContext ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&SinkRxTrace));

  lteH->EnableMacTraces ();
  lteH->EnableRlcTraces ();
  lteH->EnablePdcpTraces ();

  ThroughputMonitor(&fmH , monAll);

  Simulator::Run();

  // Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
  // std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  Simulator::Destroy();
  return 0;
}
