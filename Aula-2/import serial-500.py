import serial
import serial.tools.list_ports
import tkinter as tk
from tkinter import messagebox

#Essa porta deve ser trocada para a mesma em que o microcontrolador
PORTA = '/dev/ttyUSB0'
BAUD = 9600

# Calibração: preencha após medir leite no béquer (branco)
# e uma referência escura (preto). Valores são raw do sensor.
# Formato: (raw_branco, raw_preto) para cada canal.

######################################################
#       branco   preto
######################################################

CAL ={
    'R': (20,  150),
    'G': (20,  232),
    'B': (20,  250),
} 

def validar_calibracao():
    for canal, (branco, preto) in CAL.items():
        if branco >= preto:
            raise ValueError(
                f"Calibração inválida para canal {canal}: "
                f"branco ({branco}) deve ser menor que preto ({preto})."
            )
        if (preto - branco) < 50:
            print(
                f"Aviso: intervalo de calibração estreito para canal {canal} "
                f"({preto - branco} unidades). Considere recalibrar."
            )

def mapear(val, branco, preto):
    val = max(branco, min(preto, val))
    normalizado = (val - branco) / (preto - branco)
    return round((1 - normalizado) * 255)

def raw_para_rgb(r, g, b):
    return (
        mapear(r, *CAL['R']),
        mapear(g, *CAL['G']),
        mapear(b, *CAL['B']),
    )

def rgb_para_hex(r, g, b):
    return f'#{r:02x}{g:02x}{b:02x}'

def iniciar():
    validar_calibracao()

    try:
        ser = serial.Serial(PORTA, BAUD, timeout=2)
    except serial.SerialException:
        portas = [p.device for p in serial.tools.list_ports.comports()]
        msg = f"Porta '{PORTA}' não encontrada.\nPortas disponíveis: {portas if portas else 'nenhuma'}"
        messagebox.showerror("Erro de conexão", msg)
        return

    historico = []  # lista de (r_raw, g_raw, b_raw, hex)

    janela = tk.Tk()
    janela.title("Leitura de cor - TCS3200")
    janela.geometry("500x450")

    canvas = tk.Canvas(janela, width=500, height=150, bg='#888888')
    canvas.pack()

    label_rgb = tk.Label(janela, text="Aguardando leitura...", font=("Courier", 12))
    label_rgb.pack(pady=4)

    label_raw = tk.Label(janela, text="", font=("Courier", 10), fg="gray")
    label_raw.pack()

    frame_hist = tk.Frame(janela)
    frame_hist.pack(pady=8)

    tk.Label(frame_hist, text="Histórico (últimas 5 leituras):", font=("Courier", 10)).pack()

    historico_canvas = tk.Canvas(frame_hist, width=500, height=50)
    historico_canvas.pack()

    def atualizar_historico():
        historico_canvas.delete("all")
        n = len(historico)
        largura = 500 // 5
        for idx, (_, _, _, cor_hex) in enumerate(historico[-5:]):
            x0 = idx * largura
            historico_canvas.create_rectangle(x0, 0, x0 + largura, 50, fill=cor_hex, outline="white")

    def atualizar():
        linha = ser.readline().decode('utf-8', errors='ignore').strip()
        if linha.count(',') == 2:
            try:
                r_raw, g_raw, b_raw = map(int, linha.split(','))
                r, g, b = raw_para_rgb(r_raw, g_raw, b_raw)
                cor = rgb_para_hex(r, g, b)
                canvas.config(bg=cor)
                label_rgb.config(text=f"RGB: ({r}, {g}, {b})  HEX: {cor}")
                label_raw.config(text=f"Raw: R={r_raw}  G={g_raw}  B={b_raw}")
                historico.append((r_raw, g_raw, b_raw, cor))
                atualizar_historico()
            except ValueError:
                pass
        janela.after(500, atualizar)

    def ao_fechar():
        ser.close()
        janela.destroy()

    janela.protocol("WM_DELETE_WINDOW", ao_fechar)
    janela.after(500, atualizar)
    janela.mainloop()

if __name__ == '__main__':
    iniciar()