#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"  // Importar NetAnim
#include "ns3/mobility-module.h" // Importar módulo de movilidad

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CRANSimulation");

int main(int argc, char *argv[]) {
    // Configuración de simulación
    uint32_t numUeNodes = 20; // Número de nodos de usuario (UE)
    uint32_t numRrhNodes = 5; // Número de RRHs
    uint32_t numOnuNodes = 5; // Número de ONUs
    Time simTime = Seconds(10.0);

    // Crear nodos
    NodeContainer ueNodes, rrhNodes, onuNodes, baseStationNode, cloudNode;
    ueNodes.Create(numUeNodes);
    rrhNodes.Create(numRrhNodes);
    onuNodes.Create(numOnuNodes);
    baseStationNode.Create(1);
    cloudNode.Create(1);

    // Instalar pila de protocolos de Internet
    InternetStackHelper stack;
    stack.Install(ueNodes);
    stack.Install(rrhNodes);
    stack.Install(onuNodes);
    stack.Install(baseStationNode);
    stack.Install(cloudNode);

    // Configurar enlaces punto a punto
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    // Conectar ONUs con la estación base
    NetDeviceContainer onuToBaseDevices;
    for (uint32_t i = 0; i < numOnuNodes; ++i) {
        onuToBaseDevices.Add(p2p.Install(onuNodes.Get(i), baseStationNode.Get(0)));
    }

    // Conectar RRHs con las ONUs
    NetDeviceContainer rrhToOnuDevices;
    for (uint32_t i = 0; i < numRrhNodes; ++i) {
        rrhToOnuDevices.Add(p2p.Install(rrhNodes.Get(i), onuNodes.Get(i % numOnuNodes)));
    }

    // Conectar UEs con las RRHs
    NetDeviceContainer ueToRrhDevices;
    for (uint32_t i = 0; i < numUeNodes; ++i) {
        ueToRrhDevices.Add(p2p.Install(ueNodes.Get(i), rrhNodes.Get(i % numRrhNodes)));
    }

    // Conectar la estación base con la nube
    NetDeviceContainer baseToCloudDevices;
    baseToCloudDevices.Add(p2p.Install(baseStationNode.Get(0), cloudNode.Get(0)));

    // Asignar direcciones IP
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.255.255.0");
    ipv4.Assign(onuToBaseDevices);
    ipv4.SetBase("10.1.0.0", "255.255.255.0");
    ipv4.Assign(rrhToOnuDevices);
    ipv4.SetBase("10.2.0.0", "255.255.255.0");
    ipv4.Assign(ueToRrhDevices);
    ipv4.SetBase("10.3.0.0", "255.255.255.0");
    ipv4.Assign(baseToCloudDevices);

    // Configurar aplicaciones de tráfico
    uint16_t port = 9;

    // Servidor en la nube
    UdpEchoServerHelper echoServer(port);
    ApplicationContainer serverApps = echoServer.Install(cloudNode.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(simTime);

    // Cliente en los nodos UE
    UdpEchoClientHelper echoClient(Ipv4Address("10.3.0.1"), port);
    echoClient.SetAttribute("MaxPackets", UintegerValue(100));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(0.1)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps;
    for (uint32_t i = 0; i < numUeNodes; ++i) {
        clientApps.Add(echoClient.Install(ueNodes.Get(i)));
    }
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(simTime);

    // Configurar movilidad para los nodos
    MobilityHelper mobility;

    // Modelo de posición constante para nodos fijos
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    // Asignar movilidad a los nodos
    mobility.Install(ueNodes);
    mobility.Install(rrhNodes);
    mobility.Install(onuNodes);
    mobility.Install(baseStationNode);
    mobility.Install(cloudNode);

    // Configurar posiciones iniciales para los nodos
    for (uint32_t i = 0; i < ueNodes.GetN(); ++i) {
        Ptr<ConstantPositionMobilityModel> mob = ueNodes.Get(i)->GetObject<ConstantPositionMobilityModel>();
        mob->SetPosition(Vector(10 * i, 0, 0)); // Espaciar nodos en el eje X
    }

    for (uint32_t i = 0; i < rrhNodes.GetN(); ++i) {
        Ptr<ConstantPositionMobilityModel> mob = rrhNodes.Get(i)->GetObject<ConstantPositionMobilityModel>();
        mob->SetPosition(Vector(50, 10 * i, 0)); // Espaciar nodos en el eje Y
    }

    for (uint32_t i = 0; i < onuNodes.GetN(); ++i) {
        Ptr<ConstantPositionMobilityModel> mob = onuNodes.Get(i)->GetObject<ConstantPositionMobilityModel>();
        mob->SetPosition(Vector(100, 20 * i, 0)); // Espaciar nodos en el eje Y
    }

    Ptr<ConstantPositionMobilityModel> baseStationMob = baseStationNode.Get(0)->GetObject<ConstantPositionMobilityModel>();
    baseStationMob->SetPosition(Vector(150, 50, 0)); // Posición de la estación base

    Ptr<ConstantPositionMobilityModel> cloudMob = cloudNode.Get(0)->GetObject<ConstantPositionMobilityModel>();
    cloudMob->SetPosition(Vector(200, 50, 0)); // Posición de la nube

    // // Habilitar rastreo de paquetes
    // AsciiTraceHelper ascii;
    // p2p.EnableAsciiAll(ascii.CreateFileStream("c-ran-simulation.tr"));
    // p2p.EnablePcapAll("c-ran-simulation");

    // Poblar tablas de enrutamiento
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Crear el animador y establecer el archivo de salida XML
    AnimationInterface anim("c-ran-simulation.xml");

    // Opcional: Configurar etiquetas para nodos
    for (uint32_t i = 0; i < ueNodes.GetN(); ++i) {
        anim.UpdateNodeDescription(ueNodes.Get(i), "UE-" + std::to_string(i)); // Etiqueta para UEs
        anim.UpdateNodeColor(ueNodes.Get(i), 0, 255, 0); // Verde para UEs
    }
    for (uint32_t i = 0; i < rrhNodes.GetN(); ++i) {
        anim.UpdateNodeDescription(rrhNodes.Get(i), "RRH-" + std::to_string(i)); // Etiqueta para RRHs
        anim.UpdateNodeColor(rrhNodes.Get(i), 255, 0, 0); // Rojo para RRHs
    }
    anim.UpdateNodeDescription(baseStationNode.Get(0), "BaseStation");
    anim.UpdateNodeColor(baseStationNode.Get(0), 0, 0, 255); // Azul para la estación base
    anim.UpdateNodeDescription(cloudNode.Get(0), "Cloud");
    anim.UpdateNodeColor(cloudNode.Get(0), 255, 255, 0); // Amarillo para la nube


    // Iniciar simulación
    Simulator::Stop(simTime);
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
