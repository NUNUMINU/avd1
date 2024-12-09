import serial
import threading
import tkinter as tk
from tkinter import messagebox
import json
import pymysql
import time
from datetime import datetime

arduino = None
serial_thread = None


def open_serial():
    global arduino, serial_thread
    try:
        arduino = serial.Serial('COM3', 9600)

        # 시리얼 데이터를 읽는 스레드 시작
        serial_thread = threading.Thread(target=read_from_arduino)
        serial_thread.daemon = True  # 프로그램 종료 시 스레드도 종료됨
        serial_thread.start()
    except Exception as e:
        messagebox.showerror("Error", f"Failed to open serial port: {e}")



def close_serial():
    global arduino
    if arduino and arduino.is_open:
        arduino.close()
    else:
        messagebox.showinfo("Info", "Serial port is already closed.")


def read_from_arduino():
    global arduino
    try:
        while arduino and arduino.is_open:
            if arduino.in_waiting > 0:
                # 아두이노로부터 데이터를 읽음
                data = arduino.readline().decode('utf-8').strip()
                print(data)
                # JSON 데이터 역직렬화 시도
                try:
                    json_data = json.loads(data)
                    fetch_data(json_data['id'], json_data['machine'])
                except json.JSONDecodeError:
                    print(f"Received (not JSON): {data}\n")
    except serial.SerialException:
        print("Serial connection lost.\n")

def send_to_arduino(i_height, i_weight):
    global arduino
    if arduino and arduino.is_open:
        data_dict={
            "height" : i_height,
            "weight" : i_weight
        }

        json_data = json.dumps(data_dict) + '\n'
        arduino.write(json_data.encode('utf-8'))
    else:
        messagebox.showinfo("info", "통신이 연결 안됨")


def fetch_data(id, machine):
    try:
        # 데이터베이스에 연결
        conn = pymysql.connect(
            host="localhost",
            user="root",
            password="0000",
            database="example3",
            charset='utf8',
            cursorclass=pymysql.cursors.DictCursor
        )
        cursor = conn.cursor()

        sql = "select * from work_info where id=%s"
        cursor.execute(sql, (id,))
        results = cursor.fetchall()

        if len(results) == 0:
            messagebox.showinfo("Info", "등록되지 않은 카드")
        else:
            name = results[0]['name']
            current_time = datetime.now()
            date = current_time.strftime("%Y-%m-%d %H-%M-%S") + f".{current_time.microsecond // 1000:03d}"

            sql = "insert into work_trace(id, name, machine, date) values(%s,%s,%s,%s)"
            cursor.execute(sql, (id, name, machine, date))
            conn.commit()

        cursor.close()
        conn.close()
        send_to_arduino(results[0]['height'], results[0]['weight'])

    except pymysql.MySQLError as err:
        print(f"Error: {err}")
    except Exception as e:
        print(f"Unexpected error: {e}")


def fetch_data_from_db(tagid):
    try:
        conn = pymysql.connect(
            host="localhost",
            user="root",
            password="0000",
            database="example3",
            cursorclass=pymysql.cursors.DictCursor
        )
        cursor = conn.cursor()

        sql = "SELECT * FROM work_trace WHERE id=%s ORDER BY date DESC LIMIT 1"
        cursor.execute(sql, (tagid,))
        results = cursor.fetchone()

        cursor.close()
        conn.close()

        if results:
            return results
        else:
            return None

    except pymysql.MySQLError as err:
        print(f"Error: {err}")
        return None


def create_gui():
    root = tk.Tk()
    root.title("위치 정보 조회 시스템")

    # 레이블 생성
    label1 = tk.Label(root, text="위치 정보 조회 시스템", font=("굴림", 26))
    label1.grid(row=0, column=0, columnspan=3, pady=20)

    label2 = tk.Label(root, text="태그ID:", font=("굴림", 18))
    label2.grid(row=1, column=0, padx=10, pady=5, sticky='e')

    label2_1 = tk.Label(root, text="", font=("굴림", 18))
    label2_1.grid(row=1, column=1, pady=20)

    inputBox = tk.Entry(root, font=("굴림", 18))
    inputBox.grid(row=1, column=1, padx=10, pady=5, sticky='e')

    label3 = tk.Label(root, text="이름:", font=("굴림", 18))
    label3.grid(row=2, column=0, padx=10, pady=5, sticky='e')

    label3_1 = tk.Label(root, text="", font=("굴림", 18))
    label3_1.grid(row=2, column=1, pady=20)

    label4 = tk.Label(root, text="기구:", font=("굴림", 18))
    label4.grid(row=3, column=0, padx=10, pady=5, sticky='e')

    label4_1 = tk.Label(root, text="", font=("굴림", 18))
    label4_1.grid(row=3, column=1, pady=20)

    label5 = tk.Label(root, text="방문일시:", font=("굴림", 18))
    label5.grid(row=4, column=0, padx=10, pady=5, sticky='e')

    label5_1 = tk.Label(root, text="", font=("굴림", 18))
    label5_1.grid(row=4, column=1, pady=20)




    def on_search():
        tagid = inputBox.get()
        if not tagid:
            messagebox.showwarning("경고", "태그 ID를 입력하세요!")
            return

        record = fetch_data_from_db(tagid)

        if record:
            label3_1.config(text=f"{record['name']}")
            label4_1.config(text=f"{record['machine']}")
            label5_1.config(text=f"{record['date']}")
        else:
            messagebox.showinfo("정보", "등록된 데이터가 없습니다.")

    search_button = tk.Button(root, text="조회", font=("굴림", 16), command=on_search)
    search_button.grid(row=1, column=2, padx=10, pady=5)

    open_button = tk.Button(root, text="Open Serial Port", command=open_serial)
    open_button.grid(row=5, column=1, pady=20)

    close_button = tk.Button(root, text="Close Serial Port", command=close_serial)
    close_button.grid(row=6, column=1,pady=20)

    root.geometry("600x600")
    root.mainloop()


create_gui()
