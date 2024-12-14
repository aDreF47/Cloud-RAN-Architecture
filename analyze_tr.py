import re
import matplotlib.pyplot as plt

def process_trace(file_path):
    send_times = {}
    receive_times = {}
    latencies = []
    sent_packets = 0
    received_packets = 0
    lost_packets = 0

    with open(file_path, "r") as f:
        for line in f:
            parts = line.split()
            if len(parts) < 2:
                continue
            event = parts[0]
            time = float(parts[1])
            packet_id = parts[-1]

            if event == "+":
                send_times[packet_id] = time
                sent_packets += 1
            elif event == "r":
                if packet_id in send_times:
                    latencies.append(time - send_times[packet_id])
                    received_packets += 1
            elif event == "d":
                lost_packets += 1

    avg_latency = sum(latencies) / len(latencies) if latencies else 0
    delivery_rate = (received_packets / sent_packets) * 100 if sent_packets else 0

    print(f"Paquetes enviados: {sent_packets}")
    print(f"Paquetes recibidos: {received_packets}")
    print(f"Paquetes perdidos: {lost_packets}")
    print(f"Latencia promedio: {avg_latency:.4f} s")
    print(f"Tasa de entrega: {delivery_rate:.2f}%")

    return latencies

def plot_latencies(latencies):
    plt.plot(latencies, label="Latencia")
    plt.xlabel("Paquete")
    plt.ylabel("Latencia (s)")
    plt.title("Latencia por Paquete")
    plt.legend()
    plt.grid()
    plt.show()

trace_file = "c-ran-simulation.tr"
latencies = process_trace(trace_file)
if latencies:
    plot_latencies(latencies)
