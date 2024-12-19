import socket
import threading
import time
import struct
import numpy as np
front_data_list = [] #縦に積んでく
back_data_list = []

# サーバを動かす関数
def start_server():
    host = ''
    port = 8002

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((host, port))
    server_socket.listen(5)

    print(f"Server started at {host}:{port}")
    
    while True:
        conn, addr = server_socket.accept()
        client_thread = threading.Thread(target=handle_client, args=(conn, addr))
        client_thread.start()

# つながったら動く関数
def handle_client(conn, addr):
    with conn:
        message = "who,"
        sending(message, conn)
        time.sleep(2)
        client_name = receiving(conn)
        print(f"Received client name: {client_name}")

        if client_name == "front": # 前
            print(f"connect:{client_name}")
            message = "action"
            sending(message, conn)
            transceiver(conn, addr, 0)
            print(f"connect end: {addr}/{client_name}")
            print(front_data_list)
            print(back_data_list)

        elif client_name == "back": # 後
            print(f"connect:{client_name}")
            message = "connecting"
            sending(message, conn)
            transceiver(conn, addr, 1)
            print(f"connect end: {addr}/{client_name}")

        elif client_name == "client2": # Androidとの通信処理
            print(f"connect:{client_name}")
            while True:
                message = input("message:") # 複数の場合は,で区切る
                sending(message, conn)
                print("sending success")

                data = receiving(conn)
                print("receiving success")
                if data == "quit":
                    break
                print(data)
            print(f"通信終了: {addr}\n{client_name}")
            conn.close()

        else:
            print("誰か知らん奴接続した")
            conn.close()

# メッセージの送信
def sending(message, conn):
    if message == "":
        message = "quit,1"
    conn.sendall((message + "\n").encode('utf-8'))

# データの受信
def receiving(conn):
    data = conn.recv(1024)
    try:
        if len(data) == struct.calcsize('3d'):
            sendtime = struct.unpack('3d', data)
            second = int(sendtime[1])
            milli = int(sendtime[2])
            hzdata = np.array([sendtime[0], float(str(second) + "." + str(milli))])
            return hzdata
        else:
            data = data.split(b'\n')[0]
            data = data.decode('utf-8').strip()
            return data
    except UnicodeDecodeError:
        data = "デコードエラー"
        return data

# データを受け取り続ける関数    
def transceiver(conn, addr, micnum):
    i = 0
    while True:
        if i == 10:
            break
                
        data = receiving(conn)
        if type(data) is str == True:
            if data == "quit":
                break
            elif data == "デコードエラー":
                print(data)
                break
        if micnum == 0:
            print(f"receiving success:{data[0]}-{data[1]}")
            front_data_list.append([data[0], data[1]]) #データ格納
        else:
            print(f"receiving success:{data[0]}-{data[1]}")
            back_data_list.append([data[0], data[1]]) #データ格納

        difference = front_data_list[-1][1] - front_data_list[0][1]
        if difference > 2.0:#2秒のデータ取れたかチェック
            print("2秒以上取れた")
            result = sound_identification()
            if result == 1:
                i = 1 #Androidにメッセージ送信
        message = "action"
        sending(message, conn)
        print("sending success")
        i = i + 1
    conn.close()
    return


def sound_identification():
    serial = 5
    front_serial = 0
    back_serial = 0

    for i in range(len(front_data_list)):
        if front_data_list[i][0] != 0:
            front_serial += 1
        else:
            front_serial = 0
        
    for i in range(len(back_data_list)):
        if back_data_list[i][0] != 0:
            back_serial += 1
        else:
            back_serial = 0
    
    if front_serial > serial or back_serial > serial:
        print("サイレンや")
        return 1
    else:
        front_serial = 0
        back_serial = 0
        return 0

        
# def time_error(data_list):#誤差検知

    # data_listの中身を一時保存
    # original_list = data_list.copy()

    # M5_farst,time1 = None,None
    # M5_second,time2 =None,None

    # for i in range(len(data_list)):
    #     if data_list[i] != original_list[i]:
    #         # 配列の場所を知るため
    #         M5_farst = i+1
    #         # 変更があったdata_listの場所の秒を保存
    #         time1 = data_list[i+1]

    # for e in range(len(data_list)):
    #     if data_list[e] != original_list[e]:
    #         M5_second = i+1
    #         time2 = data_list[e+1]
    #         break
        
    #     return {
    #     "M5_farst": M5_farst,
    #     "time_error1": time1,
    #     "M5_second": M5_second,
    #     "time_error2": time2
    # }
    # # 右前と左前
    # if (M5_farst == 1 and M5_second == 3) or (M5_farst == 3 and M5_second == 1):
    #     diff_data = time2 - time1
    #     # 前からの時
    #     if -0.004 < diff_data < 0.004:
    #         send_data = 1
    #     # 右前からの時
    #     elif (M5_farst == 1 and diff_data >= 0.004) or (M5_farst == 3 and diff_data <= -0.004):
    #         send_data = 2
    #     # 左前からの時
    #     elif (M5_farst == 3 and diff_data >= 0.004) or (M5_farst == 1 and diff_data <= -0.004):
    #         send_data = 3

    # # 右前と右後
    # if (M5_farst == 1 and M5_second == 5) or (M5_farst == 5 and M5_second == 1):
    #     diff_data = time2 - time1
    #     # 右からの時
    #     if -0.008 < diff_data < 0.008:
    #         send_data = 4
    #     # 右前からの時
    #     elif (M5_farst == 1 and diff_data >= 0.008) or (M5_farst == 5 and diff_data <= -0.008):
    #         send_data = 2
    #     # 右後からの時
    #     elif (M5_farst == 5 and diff_data >= 0.008) or (M5_farst == 1 and diff_data <= -0.008):
    #         send_data = 5

    # # 右後と左後
    # if (M5_farst == 5 and M5_second == 7) or (M5_farst == 7 and M5_second == 5):
    #     diff_data = time2 - time1
    #     # 後ろからの時
    #     if -0.004 < diff_data < 0.004:
    #         send_data = 6
    #     # 右後ろからの時
    #     elif (M5_farst == 5 and diff_data >= 0.004) or (M5_farst == 7 and diff_data <= -0.004):
    #         send_data = 5
    #     # 左後からの時
    #     elif (M5_farst == 7 and diff_data >= 0.004) or (M5_farst == 5 and diff_data <= -0.004):
    #         send_data = 7

    # # 左前と左後
    # if (M5_farst == 7 and M5_second == 3) or (M5_farst == 3 and M5_second == 7):
    #     diff_data = time2 - time1
    #     # 左からの時
    #     if -0.008 < diff_data < 0.008:
    #         send_data = 8
    #     # 左前からの時
    #     elif (M5_farst == 3 and diff_data >= 0.008) or (M5_farst == 7 and diff_data <= -0.008):
    #         send_data = 3
    #     # 左後からの時
    #     elif (M5_farst == 7 and diff_data >= 0.008) or (M5_farst == 3 and diff_data <= -0.008):
    #         send_data = 7


if __name__ == "__main__":
    start_server()