#!/usr/bin/env python
# -*- coding: utf-8 -*-

import socket
import sys

def udp_echo_server(host, port):
    # Create a UDP socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # Bind the socket to a specific address and port
    server_socket.bind((host, port))
    print(f"UDP echo server is listening on {host}:{port}")

    while True:
        # Set memory to zero before each recvfrom operation
        data = bytearray(8192)
        # Receive data from the client
        received_bytes, client_address = server_socket.recvfrom_into(data)

        # Convert the received bytes to string
        received_data = data[:received_bytes].decode()
        print(f"Received data from {client_address}: {received_data}")

        # Echo the received data back to the client
        server_socket.sendto(data[:received_bytes], client_address)
        print(f"Sent data back to {client_address}: {received_data}")

    # Close the socket
    server_socket.close()

# Check if the IP address and port are provided as command-line arguments
if len(sys.argv) != 3:
    print("Usage: python server.py <ip> <port>")
else:
    host = sys.argv[1]
    port = int(sys.argv[2])
    # Run the UDP echo server
    udp_echo_server(host, port)
