import multiprocessing
from subprocess import Popen, PIPE, STDOUT
import os
import select


def start_process(num):
    print('Starting', num)
    args = ["./cot", "127.0.0.1", "5800%d" % num, "193.136.138.142", "59000"]
    p = Popen(args, stdin=PIPE, stdout=PIPE, stderr=open("err.txt", "w"))
    return p


def connect(fromm, fromid, too, toid):
    output = "\n" + "-" * 20
    try:
        fromm.stdin.flush()
        fr = fromm.communicate(
            b"djoin 037 %02d %02d 127.0.0.1 5800%01d\n" % (fromid, toid, toid))[0].decode("utf-8")
        output += "\nFrom %d:\n " % fromid+fr

        tr = too.communicate()[0].decode("utf-8")
        output += "\nTo: %d:\n " % toid + tr
    except Exception as e:
        output += str(e)
    output += "-" * 20+"\n"
    print(output)
    with open("log.txt", "a") as f:
        f.write(output)


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