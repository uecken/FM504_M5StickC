import serial
import time

def send_command(ser, command, description):
    """コマンドを送信し、応答を待つ"""
    print(f"\n=== {description} ===")
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    ser.write(command)
    print(f"Sent command: {' '.join(f'{b:02X}' for b in command)}")
    ser.flush()
    time.sleep(0.1)
    response = read_response(ser)
    return response

def send_query_epc_command(ser):
    print("\n=== Query EPC ===")
    
    # バッファクリア
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    
    # Query EPCコマンド送信
    command = b'\x0A\x51\x0D'  # <LF>Q<CR>
    ser.write(command)
    print("Sent Query EPC command: <LF>Q<CR>")
    
    ser.flush()
    time.sleep(0.1)
    
    # 応答を読み取る
    response = read_response(ser)
    if response:
        # 応答から実際のデータ部分を抽出（<LF>と<CR><LF>を除く）
        data = ''.join(chr(b) for b in response[1:-2])
        print(f"\nQuery Result:")
        print(f"  Raw data: {''.join(chr(b) if 32 <= b <= 126 else f'<{b:02X}>' for b in response)}")
        print(f"  EPC     : {data}")
        print(f"  Hex     : {' '.join(f'{b:02X}' for b in response)}")
        
        # タイムスタンプを追加
        timestamp = time.strftime("%H:%M:%S", time.localtime())
        print(f"  Time    : {timestamp}")
    return response

def read_response(ser):
    response = []
    start_time = time.time()
    
    while time.time() - start_time < 1.0:
        if ser.in_waiting:
            byte = ser.read()
            response.append(byte[0])
            print(f"Received: {byte[0]:02X} (ASCII: {chr(byte[0]) if 32 <= byte[0] <= 126 else '.'})")
            time.sleep(0.01)
        elif len(response) > 0:
            break
    
    return response

def initialize_reader(ser):
    """リーダーの初期設定を行う"""
    # JP mode 916-921MHz設定
    #jp_mode_cmd = b'\x0A\x4E\x35\x2C\x30\x36\x0D'
    #send_command(ser, jp_mode_cmd, "Set JP Mode 916-921MHz")
    
    # TW mode 922-928MHz設定
    tw_mode_cmd = b'\x0A\x4E\x35\x2C\x30\x32\x0D'
    send_command(ser, tw_mode_cmd, "Set TW Mode 922-928MHz")
    time.sleep(0.1)
    
    # 周波数設定 (<LF>J101<CR> を16進数で表現)
    #freq_cmd = b'\x0A\x4A\x31\x30\x31\x0D'
    #end_command(ser, freq_cmd, "Set Frequency")

    
    # 周波数設定 (<LF>J110<CR> を16進数で表現)
    freq_cmd = b'\x0A\x4A\x31\x30\x32\x0D'
    send_command(ser, freq_cmd, "Set Frequency")
    time.sleep(0.1)



def main(port='COM9', baudrate=38400):
    try:
        # シリアルポートを開く
        ser = serial.Serial(
            port=port,
            baudrate=baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=1
        )
        print(f"Port {port} opened successfully at {baudrate}bps")

        initialize_reader(ser)
        
        print("\nStarting continuous Query EPC (Press Ctrl+C to stop)")
        read_count = 0
        start_time = time.time()
        
        while True:
            read_count += 1
            elapsed = time.time() - start_time
            print(f"\nQuery #{read_count} (Elapsed: {elapsed:.1f}s)")
            send_query_epc_command(ser)
            time.sleep(0.1)  # 1秒待機
            
    except KeyboardInterrupt:
        print(f"\nStopping... Total queries: {read_count}")
    
    except serial.SerialException as e:
        print(f"Error: {e}")
    
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print(f"Port {port} closed")

if __name__ == "__main__":
    main('COM9')