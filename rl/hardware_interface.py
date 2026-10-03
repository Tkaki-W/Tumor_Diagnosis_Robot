"""
C++側(MotionPlayerGs2dC)とTCPで通信するだけのクラス。
float(4バイト)を1個送って、float(4バイト)を1個受け取る。
"""
import socket
import struct


class HardwareInterface:
    def __init__(self, host="127.0.0.1", port=12345):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((host, port))

    def send_action(self, delta_angle: float):
        self.sock.sendall(struct.pack("f", delta_angle))

    def receive_observation(self) -> float:
        data = self.sock.recv(4)
        return struct.unpack("f", data)[0]

    def close(self):
        self.sock.close()
