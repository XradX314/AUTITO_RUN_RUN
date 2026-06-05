import socket
import struct
import time
import threading
import tkinter as tk
from tkinter import ttk

# --- CONSTANTES DEL PROTOCOLO ---
CMD_TELEMETRIA = 0x01
CMD_ACCION     = 0x02
CMD_ALIVE      = 0x03
CMD_ALIVE_ACK  = 0x04
CMD_SET_ANGLE  = 0x05

class CentroControlGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Centro de Control - Vehículo Autónomo")
        self.root.geometry("450x400")
        self.root.resizable(False, False)

        # Variables de estado
        self.escuchando = False
        self.sock = None
        self.robot_addr = None
        self.ultimo_latido = 0

        self.armar_interfaz()
        self.decodificador = DecodificadorBinario(self.procesar_comando)

        # Iniciar el watchdog del Heartbeat
        self.verificar_conexion()

    def armar_interfaz(self):
        # --- MARCO DE CONEXIÓN ---
        frame_conn = ttk.LabelFrame(self.root, text="Ajustes de Conexión UDP")
        frame_conn.pack(fill="x", padx=10, pady=10)

        ttk.Label(frame_conn, text="Puerto local:").grid(row=0, column=0, padx=5, pady=5)
        self.entry_port = ttk.Entry(frame_conn, width=10)
        self.entry_port.insert(0, "8080")
        self.entry_port.grid(row=0, column=1, padx=5, pady=5)

        self.btn_conectar = ttk.Button(frame_conn, text="Conectar", command=self.toggle_conexion)
        self.btn_conectar.grid(row=0, column=2, padx=10, pady=5)

        # --- MARCO DE ESTADO / ALIVE ---
        frame_estado = ttk.Frame(self.root)
        frame_estado.pack(fill="x", padx=10, pady=5)
        
        ttk.Label(frame_estado, text="Estado del Enlace:", font=("Arial", 12)).pack(side="left")
        self.lbl_alive = tk.Label(frame_estado, text="DESCONECTADO", fg="red", font=("Arial", 12, "bold"))
        self.lbl_alive.pack(side="right", padx=10)

        # --- MARCO DE TELEMETRÍA ---
        frame_telemetria = ttk.LabelFrame(self.root, text="Telemetría en Tiempo Real")
        frame_telemetria.pack(fill="both", expand=True, padx=10, pady=10)

        # Configurar grilla
        for i in range(4):
            frame_telemetria.rowconfigure(i, weight=1)
        frame_telemetria.columnconfigure(1, weight=1)

        fuente_datos = ("Consolas", 14, "bold")

        ttk.Label(frame_telemetria, text="IR Izquierdo:", font=("Arial", 11)).grid(row=0, column=0, sticky="e", padx=10)
        self.lbl_ir_l = tk.Label(frame_telemetria, text="----", font=fuente_datos, fg="blue")
        self.lbl_ir_l.grid(row=0, column=1, sticky="w")

        ttk.Label(frame_telemetria, text="IR Central:", font=("Arial", 11)).grid(row=1, column=0, sticky="e", padx=10)
        self.lbl_ir_c = tk.Label(frame_telemetria, text="----", font=fuente_datos, fg="blue")
        self.lbl_ir_c.grid(row=1, column=1, sticky="w")

        ttk.Label(frame_telemetria, text="IR Derecho:", font=("Arial", 11)).grid(row=2, column=0, sticky="e", padx=10)
        self.lbl_ir_r = tk.Label(frame_telemetria, text="----", font=fuente_datos, fg="blue")
        self.lbl_ir_r.grid(row=2, column=1, sticky="w")

        ttk.Label(frame_telemetria, text="Sonar (mm):", font=("Arial", 11)).grid(row=3, column=0, sticky="e", padx=10)
        self.lbl_sonar = tk.Label(frame_telemetria, text="----", font=fuente_datos, fg="green")
        self.lbl_sonar.grid(row=3, column=1, sticky="w")

        # --- MARCO DE CONTROL MANUAL ---
        frame_control = ttk.LabelFrame(self.root, text="Control Manual")
        frame_control.pack(fill="x", padx=10, pady=5)

        ttk.Label(frame_control, text="Radar:").pack(side="left", padx=5)
        
        # El Slider va de 0 a 180
        self.slider_servo = ttk.Scale(frame_control, from_=0, to=180, orient="horizontal", length=250)
        self.slider_servo.set(90) # Arranca en el centro
        self.slider_servo.pack(side="left", padx=5, pady=10)

        # Botón para disparar el comando
        self.btn_servo = ttk.Button(frame_control, text="Mover", command=self.enviar_angulo_servo)
        self.btn_servo.pack(side="left", padx=5)

    def toggle_conexion(self):
        if not self.escuchando:
            try:
                puerto = int(self.entry_port.get())
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                self.sock.bind(("0.0.0.0", puerto))
                
                self.escuchando = True
                self.btn_conectar.config(text="Desconectar")
                self.entry_port.config(state="disabled")
                self.lbl_alive.config(text="ESPERANDO DATOS...", fg="orange")
                
                # Iniciar el hilo que escucha la red
                hilo = threading.Thread(target=self.escuchar_udp, daemon=True)
                hilo.start()
            except Exception as e:
                self.lbl_alive.config(text=f"ERROR P: {puerto}", fg="red")
        else:
            self.escuchando = False
            if self.sock:
                self.sock.close()
            self.btn_conectar.config(text="Conectar")
            self.entry_port.config(state="normal")
            self.lbl_alive.config(text="DESCONECTADO", fg="red")
            self.robot_addr = None

    def enviar_angulo_servo(self):
            # Solo mandamos si estamos conectados y sabemos la IP del autito
            if self.escuchando and self.sock and self.robot_addr:
                angulo = int(self.slider_servo.get())
                
                # struct.pack('<B', angulo) convierte el número entero en 1 byte binario puro (0-255)
                payload = struct.pack('<B', angulo)
                
                # Armamos el paquete con la cabecera, comando 0x05, el byte del ángulo y el Checksum
                paquete = self.armar_paquete(CMD_SET_ANGLE, payload)
                
                # Lo lanzamos por el aire
                self.sock.sendto(paquete, self.robot_addr)
                print(f"[>] Ángulo enviado: {angulo}°")

    def escuchar_udp(self):
        while self.escuchando:
            try:
                data, addr = self.sock.recvfrom(1024)
                self.robot_addr = addr
                for byte in data:
                    self.decodificador.procesar_byte(byte)
            except OSError:
                break # El socket se cerró

    def procesar_comando(self, cmd, params):
        if cmd == CMD_ALIVE:
            # Actualizamos el reloj de latido
            self.ultimo_latido = time.time()
            
            # Respondemos el ACK
            if self.robot_addr and self.sock:
                ack_packet = self.armar_paquete(CMD_ALIVE_ACK)
                self.sock.sendto(ack_packet, self.robot_addr)

        elif cmd == CMD_TELEMETRIA:
            if len(params) == 8:
                ir_l, ir_c, ir_r, sonar = struct.unpack('<HHHH', params)
                # Actualizamos la GUI de forma segura
                self.root.after(0, self.actualizar_pantalla_telemetria, ir_l, ir_c, ir_r, sonar)

    def actualizar_pantalla_telemetria(self, l, c, r, s):
        self.lbl_ir_l.config(text=str(l))
        self.lbl_ir_c.config(text=str(c))
        self.lbl_ir_r.config(text=str(r))
        self.lbl_sonar.config(text=str(s))

    def verificar_conexion(self):
        # Se ejecuta cada 500ms en el hilo principal
        if self.escuchando:
            if time.time() - self.ultimo_latido > 1.5:
                self.lbl_alive.config(text="ENLACE CAÍDO", fg="red")
                # Limpiar datos viejos
                self.actualizar_pantalla_telemetria("----", "----", "----", "----")
            else:
                self.lbl_alive.config(text="ONLINE \u2714", fg="green")
        
        self.root.after(500, self.verificar_conexion)

    def armar_paquete(self, cmd, payload=b""):
        length = 1 + len(payload) + 1
        paquete = bytearray(b'UNER')
        paquete.append(length)
        paquete.append(ord(':'))
        paquete.append(cmd)
        paquete.extend(payload)
        
        cks = 0
        for b in paquete: cks ^= b
        paquete.append(cks)
        return paquete

class DecodificadorBinario:
    def __init__(self, callback):
        self.state = 0
        self.cks = 0
        self.nBytes = 0
        self.payload = bytearray()
        self.callback = callback

    def procesar_byte(self, b):
        if self.state == 0:
            if b == ord('U'):
                self.state = 1; self.cks = b
        elif self.state == 1:
            if b == ord('N'):
                self.state = 2; self.cks ^= b
            else: self.state = 0
        elif self.state == 2:
            if b == ord('E'):
                self.state = 3; self.cks ^= b
            else: self.state = 0
        elif self.state == 3:
            if b == ord('R'):
                self.state = 4; self.cks ^= b
            else: self.state = 0
        elif self.state == 4:
            self.nBytes = b; self.cks ^= b; self.state = 5
            self.payload = bytearray()
        elif self.state == 5:
            if b == ord(':'):
                self.cks ^= b; self.state = 6
            else: self.state = 0
        elif self.state == 6:
            self.cks ^= b
            if self.nBytes > 1: self.payload.append(b)
            self.nBytes -= 1
            if self.nBytes == 0:
                if self.cks == 0:
                    self.callback(self.payload[0], self.payload[1:])
                self.state = 0

if __name__ == "__main__":
    root = tk.Tk()
    app = CentroControlGUI(root)
    root.mainloop()