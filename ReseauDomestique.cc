#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"


// Network Topology
//
//                  Ordi1          *                     serverHTTP
//                               *                     -
//                  Ordi2      *     *       10.1.1.0 -
//                            *    *    *            -      
//   10.1.3.0                *    *    *     AP-n0  - pointToPoint
//                           *    *    *             -       
//                            *    *    *   10.1.2.0  -
//                             *     *                 -   
//                  TvIP         *                       ServerRSTP
//                                 *
//
//
//



using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThreeGppHttpExample");

void CalculateThroughput(const Ptr<RtspClient> client)
{
  if(Simulator::IsFinished() || Simulator::Now().GetSeconds() > 21) return;

  static uint64_t lastRx = 0;
  uint64_t curRx = client->GetRxSize();

  NS_LOG_INFO(Simulator::Now().GetSeconds() <<' '<<(curRx - lastRx) * 80.0 / 1000000 );
  lastRx = curRx;
  Simulator::Schedule(MilliSeconds(100), CalculateThroughput, client);
}

 void OnChangeFractionLoss(float& fractionLoss)
 {
   NS_LOG_INFO(Simulator::Now().GetSeconds() <<' '<<fractionLoss * 100);
 }

void OnChangeCongestionLevel(double& congestionLevel)
{
  NS_LOG_INFO(Simulator::Now().GetSeconds() <<' '<< congestionLevel);
}

// HTTP
void
ServerConnectionEstablished (Ptr<const ThreeGppHttpServer>, Ptr<Socket>)
{
  NS_LOG_INFO ("Client has established a connection to the server.");
}

void
MainObjectGenerated (uint32_t size)
{
  NS_LOG_INFO ("Server generated a main object of " << size << " bytes.");
}

void
EmbeddedObjectGenerated (uint32_t size)
{
  NS_LOG_INFO ("Server generated an embedded object of " << size << " bytes.");
}

void
ServerTx (Ptr<const Packet> packet)
{
  NS_LOG_INFO ("Server sent a packet of " << packet->GetSize () << " bytes.");
}

void
ClientTx (Ptr<const Packet> packet)
{
  NS_LOG_INFO ("Client sent a packet of " << packet->GetSize () << " bytes.");
}

void
ClientRx (Ptr<const Packet> packet, const Address &address)
{
  NS_LOG_INFO ("Client received a packet of " << packet->GetSize () << " bytes from " << address);
}

void
ServerRx (Ptr<const Packet> packet, const Address &address)
{
  NS_LOG_INFO ("Server received a packet of " << packet->GetSize () << " bytes from " << address);
}

void
ClientMainObjectReceived (Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
{
  Ptr<Packet> p = packet->Copy ();
  ThreeGppHttpHeader header;
  p->RemoveHeader (header);
  if (header.GetContentLength () == p->GetSize ()
      && header.GetContentType () == ThreeGppHttpHeader::MAIN_OBJECT)
    {
      NS_LOG_INFO ("Client has successfully received a main object of "
                   << p->GetSize () << " bytes.");
    }
  else
    {
      NS_LOG_INFO ("Client failed to parse a main object. ");
    }
}

void
ClientEmbeddedObjectReceived (Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
{
  Ptr<Packet> p = packet->Copy ();
  ThreeGppHttpHeader header;
  p->RemoveHeader (header);
  if (header.GetContentLength () == p->GetSize ()
      && header.GetContentType () == ThreeGppHttpHeader::EMBEDDED_OBJECT)
    {
      NS_LOG_INFO ("Client has successfully received an embedded object of "
                   << p->GetSize () << " bytes.");
    }
  else
    {
      NS_LOG_INFO ("Client failed to parse an embedded object. ");
    }
}


int 
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nWifi = 3;
  bool tracing = true;
  double simTimeSec = 300;

  CommandLine cmd;
  cmd.AddValue ("SimulationTime", "Length of simulation in seconds.", simTimeSec);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

  cmd.Parse (argc,argv);

  Time::SetResolution (Time::NS);
  LogComponentEnableAll (LOG_PREFIX_TIME);
  LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnable ("ThreeGppHttpClient", LOG_INFO);
  LogComponentEnable ("ThreeGppHttpServer", LOG_INFO);
  LogComponentEnable ("ThreeGppHttpExample", LOG_INFO);

 
  Address serverIptvAddress;
  Address clientIptvAddress;
  
  NodeContainer p2pNodes;
  p2pNodes.Create (4);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  
  NodeContainer p2pHttpNodes;
  p2pHttpNodes.Add (p2pNodes.Get(0));
  p2pHttpNodes.Add (p2pNodes.Get(1));
  
  NetDeviceContainer p2pHttpDevices;
  p2pHttpDevices = pointToPoint.Install (p2pHttpNodes);
  
  NodeContainer p2pIptvNodes;
  p2pIptvNodes.Add (p2pNodes.Get(0));
  p2pIptvNodes.Add (p2pNodes.Get(2));
  
  NetDeviceContainer p2pIptvDevices;
  p2pIptvDevices = pointToPoint.Install (p2pIptvNodes);
  
  
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode = p2pNodes.Get (0);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy ;
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211ac);
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue ("VhtMcs9"), "ControlMode", StringValue ("VhtMcs0"));

  WifiMacHelper mac;
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

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

  InternetStackHelper stack;
  stack.Install (p2pNodes);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pHttpInterfaces;
  p2pHttpInterfaces = address.Assign (p2pHttpDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pIptvInterfaces;
  p2pIptvInterfaces = address.Assign (p2pIptvDevices);

  Ipv4Address serverHttpAddress = p2pHttpInterfaces.GetAddress (1);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  
  Ipv4InterfaceContainer staInterfaces;
  staInterfaces = address.Assign (staDevices);
  Ipv4InterfaceContainer apInterfaces;
  apInterfaces = address.Assign (apDevices);
  
  clientIptvAddress = Address (staInterfaces.GetAddress (2));
  serverIptvAddress = Address (p2pIptvInterfaces.GetAddress (1));


// IPTV Configuration CLIENT SERVER

 RtspServerHelper server(serverIptvAddress);
  ApplicationContainer apps = server.Install (p2pIptvNodes.Get (1));
  server.SetAttribute("UseCongestionThreshold", BooleanValue(true));

  Ptr<RtspServer> rtspServer = DynamicCast<RtspServer>(apps.Get(0));
  rtspServer->TraceConnectWithoutContext("CongestionLevel", MakeCallback(&OnChangeCongestionLevel));

  apps.Start (Seconds (5.0));
  apps.Stop (Seconds (30.0));

  RtspClientHelper client(serverIptvAddress, clientIptvAddress);
  client.SetAttribute ("FileName", StringValue ("./scratch/frame.txt")); // set File name
  apps = client.Install (wifiStaNodes.Get (2));


  Ptr<RtspClient> rtspClient = DynamicCast<RtspClient>(apps.Get(0));
  rtspClient->ScheduleMessage(Seconds(5), RtspClient::SETUP);
  rtspClient->ScheduleMessage(Seconds(6), RtspClient::PLAY);
  rtspClient->ScheduleMessage(Seconds(10), RtspClient::PAUSE);
  rtspClient->ScheduleMessage(Seconds(13), RtspClient::PLAY);


  rtspClient->TraceConnectWithoutContext("FractionLoss", MakeCallback(&OnChangeFractionLoss));

  apps.Start (Seconds (5.0));
  apps.Stop (Seconds (30.0));

  

//  HTTP client server configuration 
  ThreeGppHttpServerHelper serverHelper (serverHttpAddress);

  ApplicationContainer serverApps = serverHelper.Install (p2pHttpNodes.Get (1));
  Ptr<ThreeGppHttpServer> httpServer = serverApps.Get (0)->GetObject<ThreeGppHttpServer> ();

  httpServer->TraceConnectWithoutContext ("ConnectionEstablished",
                                          MakeCallback (&ServerConnectionEstablished));
  httpServer->TraceConnectWithoutContext ("MainObject", MakeCallback (&MainObjectGenerated));
  httpServer->TraceConnectWithoutContext ("EmbeddedObject", MakeCallback (&EmbeddedObjectGenerated));
  httpServer->TraceConnectWithoutContext ("Tx", MakeCallback (&ServerTx));

  PointerValue varPtr;
  httpServer->GetAttribute ("Variables", varPtr);
  Ptr<ThreeGppHttpVariables> httpVariables = varPtr.Get<ThreeGppHttpVariables> ();
  httpVariables->SetMainObjectSizeMean (102400);
  httpVariables->SetMainObjectSizeStdDev (40960);

  serverApps.Start (Seconds (0));
  serverApps.Stop (Seconds (300.0));

  ThreeGppHttpClientHelper clientHelper (serverHttpAddress);

  ApplicationContainer clientApps = clientHelper.Install (wifiStaNodes.Get (0));
  Ptr<ThreeGppHttpClient> httpClient = clientApps.Get (0)->GetObject<ThreeGppHttpClient> ();
  
  PointerValue variablePtr;
  httpClient->GetAttribute ("Variables", variablePtr);
  Ptr<ThreeGppHttpVariables> httpClVariables = varPtr.Get<ThreeGppHttpVariables> ();
  httpClVariables->SetRequestSize(uint32_t  (402));
  httpClVariables->SetRequestIntervall(Time (10));

  ApplicationContainer clientApps1 = clientHelper.Install (wifiStaNodes.Get (1));
  Ptr<ThreeGppHttpClient> httpClient1 = clientApps1.Get (0)->GetObject<ThreeGppHttpClient> ();
  
  PointerValue variablePtr1;
  httpClient1->GetAttribute ("Variables", variablePtr1);
  Ptr<ThreeGppHttpVariables> httpClVariables1 = varPtr.Get<ThreeGppHttpVariables> ();
  httpClVariables1->SetRequestSize(uint32_t  (500));
  httpClVariables1->SetRequestIntervall(Time (5.0));

  httpClient->TraceConnectWithoutContext ("RxMainObject", MakeCallback (&ClientMainObjectReceived));
  httpClient->TraceConnectWithoutContext ("RxEmbeddedObject", MakeCallback (&ClientEmbeddedObjectReceived));
  httpClient->TraceConnectWithoutContext ("Rx", MakeCallback (&ClientRx));

  clientApps.Start (Seconds (1.0));
  clientApps.Stop (Seconds (300.0));
  
  clientApps1.Start (Seconds (3.0));
  clientApps1.Stop (Seconds (300.0));


  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();



  if (tracing == true)
    {
      pointToPoint.EnablePcapAll ("pointToPointTrace");
      phy.EnablePcap ("PhyTrace", apDevices.Get (0));
    }

  Simulator::Stop(Seconds(300.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
