"""
学習済みの方策(pinch_ppo_model)を読み込んで、実機で動かすだけのスクリプト。
学習はしない(ランダムな探索なしで、方策が一番良いと思う角度だけを使う)。
先にMotionPlayerGs2dC.exeをRL_TRAINING_MODE=1で実行しておくこと。
"""
from stable_baselines3 import PPO

from hardware_interface import HardwareInterface
from pinch_env import PinchEnv

MODEL_PATH = "pinch_ppo_model"
NUM_EPISODES = 15  # C++側(MotionPlayerGs2dC.cpp)の motionMax と必ず同じ値にすること


def play():
    hw = HardwareInterface()
    env = PinchEnv(hw)
    model = PPO.load(MODEL_PATH)

    for episode in range(NUM_EPISODES):
        obs, _ = env.reset()
        done = False
        while not done:
            # deterministic=True: ランダムに探索せず、方策が一番良いと思う角度だけを使う
            action, _ = model.predict(obs, deterministic=True)
            obs, reward, done, truncated, info = env.step(action)
        print(f"Episode {episode + 1}/{NUM_EPISODES} finished")

    hw.close()


if __name__ == "__main__":
    play()
