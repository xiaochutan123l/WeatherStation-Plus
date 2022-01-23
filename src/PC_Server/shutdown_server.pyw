#!usr/bin/python
# -*- coding: utf-8 -*-

import socket
import os
import time

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

host = "192.168.2.118"
port = 9998

server.bind((host, port))

server.listen(1)
print("wait for client")
client, addr = server.accept()
print(f"accept new connection from {client}:{addr}")

while True:
    msg = client.recv(4096)
    #TODO: add message identifier.
    if msg:
        print(f"msg: {msg}")
        break

server.close()
print("prepare to shutdown in 3s")
#time.sleep(10)

#TODO: add keybord interrupt exception.
cmd_str = "cd C:\windows\System32"
shutdown_str = "shutdown -s -t 1"
if not os.system(cmd_str):
    os.system(shutdown_str)

