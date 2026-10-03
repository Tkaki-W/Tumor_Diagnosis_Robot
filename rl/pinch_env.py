"""
つまむ動作の残差RL用の環境(gym.Env)。
1エピソード = C++側の RunPinchRLEpisode() 1回分。
"""
import gymnasium as gym
from gymnasium import spaces
import numpy as np

# C++側の RL_FINE_TUNE_STEPS(MotionPlayerGs2dC.cpp)と必ず同じ値にすること
MAX_STEPS = 20

# 行動(-1〜1)を実際の角度補正値[度]に広げるときの倍率
MAX_DELTA_DEG = 5.0


class PinchEnv(gym.Env):
    def __init__(self, hardware_interface):
        super().__init__()
        self.hw = hardware_interface

        self.action_space = spaces.Box(low=-1.0, high=1.0, shape=(1,), dtype=np.float32)
        # 観測: 力センサの値(今はC++側が仮の0.0を返すだけ)
        self.observation_space = spaces.Box(low=-np.inf, high=np.inf, shape=(1,), dtype=np.float32)

        self.current_step = 0

    def reset(self, seed=None, options=None):
        super().reset(seed=seed)
        self.current_step = 0
        # C++側はまだ何も送ってこないので、最初の観測はゼロ埋め
        return np.array([0.0], dtype=np.float32), {}

    def step(self, action):
        self.current_step += 1

        delta_angle = float(action[0]) * MAX_DELTA_DEG
        self.hw.send_action(delta_angle)
        force = self.hw.receive_observation()

        obs = np.array([force], dtype=np.float32)
        reward = 0.0  # TODO: センサと報酬の中身が決まったら実装する
        done = self.current_step >= MAX_STEPS

        return obs, reward, done, False, {}
