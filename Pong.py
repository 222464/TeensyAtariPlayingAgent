import gym
import numpy as np
import tinyscaler
import cv2
import serial
import struct
import time
import os

ser = serial.Serial("/dev/serial/by-id/usb-Teensyduino_USB_Serial_7824900-if00", baudrate=2000000)

time.sleep(1.0)

print("Connected.")

# Create the environment
env = gym.make('Pong-v4', render_mode='human')

minSize = min(env.observation_space.shape[0], env.observation_space.shape[1])
maxSize = max(env.observation_space.shape[0], env.observation_space.shape[1])
cropHeightOffset = 16

imgSize = (32, 32)
numActions = env.action_space.n

reward = 0.0
maxTotalReward = 0.0
averageTotalReward = 0.0

for episode in range(100000):
    obs, info = env.reset()

    totalReward = 0.0

    # Timesteps
    for t in range(10000):
        obs = obs[maxSize // 2 - minSize // 2 + cropHeightOffset : maxSize // 2 + minSize // 2 + cropHeightOffset, :, :]

        obs = tinyscaler.scale(obs, imgSize)

        obs = cv2.cvtColor(obs, cv2.COLOR_RGB2GRAY)

        data = bytearray(obs.tobytes())
        data += struct.pack("<f", reward)

        assert len(data) == (32 * 32 + 4)
        
        ser.write(data)

        buf = bytes()

        while ser.in_waiting < 5:
            time.sleep(0.001)

        while len(buf) < 5:
            buf += ser.read(5 - len(buf))

        action, check = struct.unpack("<Bf", buf)
        action = int(action)

        if abs(check - reward) > 0.01: # Not exactly
            print("Check failed! Expected " + str(reward) + ", received " + str(check))

        obs, reward, term, trunc, info = env.step(action)

        totalReward += reward

        if term or trunc:
            if episode == 0:
                maxTotalReward = totalReward
                averageTotalReward = totalReward
            else:
                maxTotalReward = max(maxTotalReward, totalReward)
                averageTotalReward = 0.99 * averageTotalReward + 0.01 * totalReward

            print("Episode {} finished after {} timesteps, gathering {} reward. Highest so far: {} Average: {}".format(episode + 1, t + 1, totalReward, maxTotalReward, averageTotalReward))

            break
