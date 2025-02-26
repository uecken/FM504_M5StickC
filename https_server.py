from http.server import HTTPServer, SimpleHTTPRequestHandler
import ssl

port = 4443
httpd = HTTPServer(('0.0.0.0', port), SimpleHTTPRequestHandler)

# 証明書の設定を修正
context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.load_cert_chain('server.pem')  # パスワードパラメータを削除

# ソケットをラップ
httpd.socket = context.wrap_socket(httpd.socket, server_side=True)
print(f"Serving HTTPS on port {port}")
print(f"Access at https://localhost:{port}")
httpd.serve_forever()