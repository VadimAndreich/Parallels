import argparse
import time
import multiprocessing
import queue
import threading

import cv2
from ultralytics import YOLO


# 1 - 103.28
# 2 - 84.86
# 4 - 88.62
# 8 - 91.9
# 12 - 101.06

def yolo_predict(in_queue, out_queue, stop_event):
    model = YOLO("yolov8s-pose.pt")
    while not stop_event.is_set():
        if in_queue.empty():
            time.sleep(0.1)
            continue
        out_queue.put(model.predict(in_queue.get(), verbose=False, device="cpu")[0].plot())


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Process video with yolov8s-pose model using multiprocessing.")
    parser.add_argument("--num_threads", "-t", type=int, default=1,
                        help="Number of threads for multiprocessing.")
    parser.add_argument("--input_file", "-i", type=str, help="Path to the input video.")
    parser.add_argument("--output_file", "-o", type=str, help="Output video name.")

    args = parser.parse_args()

    num_threads = args.num_threads
    input_file = args.input_file
    output_file = args.output_file

    in_video = cv2.VideoCapture(input_file)

    stop_event = multiprocessing.Event()
    in_queues = [queue.Queue() for i in range(num_threads)]
    out_queues = [queue.Queue() for i in range(num_threads)]
    threads = [threading.Thread(target=yolo_predict, args=(in_queues[i], out_queues[i], stop_event))
               for i in range(num_threads)]

    for i in range(num_threads):
        threads[i].start()

    num_frames = 0
    start_time = time.time()

    while True:
        res, frame = in_video.read()
        if not res:
            break
        in_queues[num_frames % num_threads].put(frame)
        num_frames += 1

    out_video = cv2.VideoWriter(output_file, cv2.VideoWriter_fourcc(*'mp4v'), 30,
                                (int(in_video.get(cv2.CAP_PROP_FRAME_WIDTH)),
                                 int(in_video.get(cv2.CAP_PROP_FRAME_HEIGHT))))

    for i in range(num_frames):
        frame = out_queues[i % num_threads].get()
        out_video.write(frame)

    stop_event.set()
    end_time = time.time()

    for i in range(num_threads):
        threads[i].join()
    in_video.release()
    out_video.release()

    print(f"Time taken to process the video: {round(end_time - start_time, 2)} seconds")
