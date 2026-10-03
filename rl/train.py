"""
PPOでつまむ動作の残差方策を学習するスクリプト。
先にMotionPlayerGs2dC.exeをRL_TRAINING_MODE=1でビルド・実行してから、このスクリプトを動かすこと。
"""
import csv

from stable_baselines3 import PPO
from stable_baselines3.common.vec_env import DummyVecEnv
from stable_baselines3.common.callbacks import BaseCallback

from hardware_interface import HardwareInterface
from pinch_env import PinchEnv

LOG_PATH = "episode_log.csv"


# 1エピソードごとの合計報酬をCSVに記録するだけのコールバック
class EpisodeLogger(BaseCallback):
    def __init__(self):
        super().__init__()
        self.episode_reward = 0.0
        with open(LOG_PATH, "w", newline="", encoding="utf-8") as f:
            csv.writer(f).writerow(["episode_reward"])

    def _on_step(self) -> bool:
        self.episode_reward += self.locals["rewards"][0]
        if self.locals["dones"][0]:
            with open(LOG_PATH, "a", newline="", encoding="utf-8") as f:
                csv.writer(f).writerow([self.episode_reward])
            self.episode_reward = 0.0
        return True


def train():
    hw = HardwareInterface()
    env = DummyVecEnv([lambda: PinchEnv(hw)])

    # n_steps: 学習を1回更新する前に、実機でつまむ動作を何ステップ分集めるか
    #          (60 = つまむ動作3回ぶん。実機なので小さめにして様子を見ながら進める)
    # batch_size: 集めたデータを何個ずつのまとまりで学習に使うか
    # learning_rate: 前回のコードに合わせて少し控えめな値にしている
    model = PPO(
        "MlpPolicy",
        env,
        n_steps=60,
        batch_size=20,
        learning_rate=1e-4,
        verbose=1,
    )
    # C++側(MotionPlayerGs2dC.cpp)の motionMax(15回)× MAX_STEPS(20) に合わせている
    model.learn(total_timesteps=300, callback=EpisodeLogger())

    model.save("pinch_ppo_model")
    hw.close()


if __name__ == "__main__":
    train()
