import serial
import serial.tools.list_ports
import tkinter as tk
from tkinter import messagebox

PORTA = '/dev/ttyUSB0'
BAUD = 9600

# Preencha com os valores raw medidos para cada cor.
# Aponte cada LED diretamente ao sensor e anote os valores exibidos em "Raw:".
REFERENCIAS = {
    'vermelho': (2, 50, 2),
    'verde':    (15, 2, 1),
    'azul':     (55, 5, 0),
    'branco':   (10, 10, 5),
    'preto':    (30000, 200000, 33000),
}

# Cores HTML exibidas no canvas para cada referência classificada
COR_DISPLAY = {
    'vermelho': '#ff0000',
    'verde':    '#00ff00',
    'azul':     '#0000ff',
    'branco':   '#ffffff',
    'preto':    '#111111',
}

def distancia(a, b):
    return sum((x - y) ** 2 for x, y in zip(a, b)) ** 0.5

def classificar(r_raw, g_raw, b_raw):
    leitura = (r_raw, g_raw, b_raw)
    return min(REFERENCIAS, key=lambda cor: distancia(leitura, REFERENCIAS[cor]))

def rgb_para_hex(r, g, b):
    return f'#{r:02x}{g:02x}{b:02x}'

def iniciar():
    try:
        ser = serial.Serial(PORTA, BAUD, timeout=2)
    except serial.SerialException:
        portas = [p.device for p in serial.tools.list_ports.comports()]
        msg = f"Porta '{PORTA}' não encontrada.\nPortas disponíveis: {portas if portas else 'nenhuma'}"
        messagebox.showerror("Erro de conexão", msg)
        return

    historico = []  # lista de (r_raw, g_raw, b_raw, cor_display_hex)

    janela = tk.Tk()
    janela.title("Leitura de cor - TCS3200")
    janela.geometry("500x500")

    canvas = tk.Canvas(janela, width=500, height=150, bg='#888888')
    canvas.pack()

    label_cor = tk.Label(janela, text="Aguardando leitura...", font=("Courier", 14))
    label_cor.pack(pady=4)

    label_raw = tk.Label(janela, text="", font=("Courier", 10), fg="gray")
    label_raw.pack()

    frame_hist = tk.Frame(janela)
    frame_hist.pack(pady=8)
    tk.Label(frame_hist, text="Histórico (últimas 5 leituras):", font=("Courier", 10)).pack()
    historico_canvas = tk.Canvas(frame_hist, width=500, height=50)
    historico_canvas.pack()

    label_hist_nomes = tk.Label(frame_hist, text="", font=("Courier", 9), fg="gray")
    label_hist_nomes.pack()

    def atualizar_historico():
        historico_canvas.delete("all")
        largura = 500 // 5
        ultimos = historico[-5:]
        nomes = []
        for idx, (_, _, _, cor_hex, nome) in enumerate(ultimos):
            x0 = idx * largura
            historico_canvas.create_rectangle(x0, 0, x0 + largura, 50, fill=cor_hex, outline="white")
            nomes.append(nome)
        label_hist_nomes.config(text="  |  ".join(nomes))

    def atualizar():
        linha = ser.readline().decode('utf-8', errors='ignore').strip()
        if linha.count(',') == 2:
            try:
                r_raw, g_raw, b_raw = map(int, linha.split(','))
                nome = classificar(r_raw, g_raw, b_raw)
                cor_hex = COR_DISPLAY[nome]
                canvas.config(bg=cor_hex)
                label_cor.config(text=f"Cor: {nome}")
                label_raw.config(text=f"Raw: R={r_raw}  G={g_raw}  B={b_raw}")
                historico.append((r_raw, g_raw, b_raw, cor_hex, nome))
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