import subprocess
import time

p = []

def opencot(n):
    cot_process = subprocess.Popen(['./cot', "127.0.0.1", "580%02i" %
                                   n, "193.136.138.142", "59000"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    return cot_process

def sendinput(s_tosend, process):
    process.stdin.flush()
    process.stdin.write(s_tosend.encode('utf-8'))
    time.sleep(1)

def main():
    for i in range(3, 10):
        p.append((opencot(i), i))
    for i in range(len(p)):
        sendinput("djoin 037 %02i 02 127.0.0.1 58002" % p[i][1], p[i][0])
        output, errors = p[i][0].communicate()
        print(f"Output from cot process {p[i][1]}:\n{output}")
        print(f"Errors from cot process {p[i][1]}:\n{errors}")
    input("wait")

if __name__ == '__main__':
    main()
