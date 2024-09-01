import queue
import time
import threading
import argparse
import logging
import os
import sys

import cv2
import keyboard


if not os.path.isdir("log"):
     os.mkdir("log")
logging.basicConfig(filename='log/app.log', level=logging.ERROR, format='%(asctime)s - %(levelname)s - %(message)s')


class CameraNameError(Exception):
    pass


class InvalidResolution(Exception):
    pass


class USBCameraDisconnected(Exception):
    pass


class Sensor:
    def get(self):
        raise NotImplementedError('Subclasses must implement method get()')


class SensorX(Sensor):
    def __init__(self, delay):
        self._delay = delay
        self._data = 0

    def get(self):
        time.sleep(self._delay)
        self._data += 1

        return self._data


class SensorCam(Sensor):
    def __init__(self, name, resol):
        self._name = name
        self._resol = resol
        self._cam = cv2.VideoCapture(0)

        if not self._cam.isOpened():
            self._cam.release()
            raise CameraNameError()

    def get(self):
        res, frame = self._cam.read()

        if res is False:
            raise USBCameraDisconnected()

        frame = cv2.resize(frame, self._resol)

        return frame

    def __del__(self):
        self._cam.release()


class WindowImage:
    def __init__(self, fps):
        self._freq = 1000 // fps
        self._name = 'video'

    def show(self, img):
        cv2.waitKey(self._freq)
        cv2.imshow(self._name, img)

    def __del__(self):
        cv2.destroyAllWindows()


def loop(sensor, queue, event_stop):
    while not event_stop.is_set():
        a = sensor.get()

        if not queue.empty():
            queue.get()

        queue.put(a)


def isValid(str):
    p = str.find('x')
    if p < 0:
        raise InvalidResolution
    else:
        try:
            int(str[:p])
            int(str[p+1:])
        except ValueError:
            raise InvalidResolution


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Camera with 3 sensors")
    parser.add_argument("--camera", "-c", type=str, required=True, help="Camera system name")
    parser.add_argument("--resolution", "-r", type=str, required=True, help="Output video resolution. Format: 1280x720")
    parser.add_argument("--framerate", "-f", type=int, required=True, help="Output video framerate")

    args = parser.parse_args()

    try:
        isValid(args.resolution)
    except InvalidResolution:
        logging.error("Incorrect resolution format. Correct format example: \"1280x720\"")
        sys.exit(2)

    try:
        camera = SensorCam(args.camera,
                           [int(args.resolution[:args.resolution.find('x')]),
                            int(args.resolution[args.resolution.find('x') + 1:])])
    except CameraNameError:
        logging.error("Camera Not Found")
        sys.exit(1)

    window = WindowImage(args.framerate)

    rates = [0, 0, 0]
    queues = [queue.Queue(maxsize=100), queue.Queue(maxsize=100), queue.Queue(maxsize=10)]
    sensors = [SensorX(0.01), SensorX(0.1), SensorX(1)]
    threads = []

    event_stop = threading.Event()

    threads.append(threading.Thread(target=loop, args=(sensors[0], queues[0], event_stop)))
    threads.append(threading.Thread(target=loop, args=(sensors[1], queues[1], event_stop)))
    threads.append(threading.Thread(target=loop, args=(sensors[2], queues[2], event_stop)))

    for t in threads:
        t.start()

    font = cv2.FONT_HERSHEY_SIMPLEX
    place = (1100, 650)
    fontScale = 0.5
    color = (0, 0, 0)
    thickness = 1

    while True:
        try:
            frame = camera.get()
        except USBCameraDisconnected:
            logging.error("USB device was disconnected. Please reconnect it.")

        while not queues[0].empty():
            rates[0] = queues[0].get()

        while not queues[1].empty():
            rates[1] = queues[1].get()

        while not queues[2].empty():
            rates[2] = queues[2].get()

        cv2.rectangle(frame, (1080, 620), (1280, 720), (255, 255, 255), thickness=-1)

        frame = cv2.putText(frame, f'Sensor0: {rates[0]}', place, font,
                            fontScale, color, thickness, cv2.LINE_AA)

        frame = cv2.putText(frame, f'Sensor1: {rates[1]}', (place[0], place[1] + 25), font,
                            fontScale, color, thickness, cv2.LINE_AA)

        frame = cv2.putText(frame, f'Sensor2: {rates[2]}', (place[0], place[1] + 50), font,
                            fontScale, color, thickness, cv2.LINE_AA)

        window.show(frame)

        if keyboard.is_pressed('q'):
            event_stop.set()
            camera.__del__()
            window.__del__()

            for t in threads:
                t.join()
            break

