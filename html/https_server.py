import http.server
import ssl
import os

def run_https_server():
    # サーバーの設定
    server_address = ('', 4443)  # ポート4443を使用
    httpd = http.server.HTTPServer(server_address, http.server.SimpleHTTPRequestHandler)
    
    # SSL/TLS設定
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(certfile='cert.pem', keyfile='key.pem')
    httpd.socket = context.wrap_socket(httpd.socket, server_side=True)
    
    print(f"Serving HTTPS on {server_address[0]}:{server_address[1]} (https://localhost:4443)")
    httpd.serve_forever()

if __name__ == "__main__":
    # htmlディレクトリに移動
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    run_https_server() 