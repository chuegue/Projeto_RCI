import multiprocessing
from subprocess import Popen, PIPE, STDOUT
import os


def start_process(num):
    # ./cot 127.0.0.1 5800%d 193.136.138.142 59000
    print('Starting', num)
    # os.system("./cot 127.0.0.1 5800%1d 193.136.138.142 59000"%num)
    p = Popen(["./cot"],args=["127.0.0.1", "5800%1d"%num, "193.136.138.142", "59000"], shell=False, stdin=PIPE, stdout=PIPE, stderr=STDOUT)
    for _ in p.stdout:
        pass
    return p


# djoin 037 %2d %2d(id onde ligar) 127.0.0.1 5800%1d(onde ligar)


def connect(fromm, fromid, too, toid):
    fromm.communicate(
        input=b"djoin 037 %2d %2d 127.0.0.1 5800%1d" % (fromid, toid, toid))[0]
    print(fromid + ":\n")
    for line in fromm.stdout:
        print(line)
    print(toid + ":")
    for line in too.stdout:
        print(line)


def main():
    processes = []
    num = int(input("Number of processes: "))
    for i in range(1, num+1):
        processes.append(start_process(i))

    while (1):
        fromid = int(input("From: "))
        toid = int(input("To: "))
        connect(processes[fromid-1], fromid, processes[toid-1], toid)


if __name__ == '__main__':
    main()