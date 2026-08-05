import serial
from time import sleep
import re
import keyboard
from collections import deque
import numpy as np


def rodar(mapa_teclas, porta: str, baudrate: int = 9600, timeout=None, n_media: int = 5, limiar: float = 4000):
    """Função que faz todo o trabalho. Prepara a comunição entre o computador e a placa. Carrega os buffers. Faz a média. E possui a rotina principal de leitra."""



    # =================================================================
    # ========== Conectar a ESP32 ao computador via Serial ==========
    # =================================================================
    esp32 = serial.Serial(port=porta, baudrate=baudrate, timeout=timeout)

    sleep(0.5) # Mensagem bonita
    print(f'[ESP32]: Conectando na porta {porta}...')
    sleep(2.5) # Tempo para garanti que a conxão foi aberta
    print(f'-- ESP32 conectada na porta {porta}. Canal Serial aberto --')



    # ===========================================================================
    # ========== Cria as variáveis importantes para resolver o problema ==========
    # ===========================================================================
    # - Buffers --> "Listas" que guardam um número de dados para que estes possam ser usados na média
    # - em_pressao --> Variável para "lembrar" quais teclas estão sendo pressionadas
    buffers = [
            deque(maxlen=n_media),
            deque(maxlen=n_media),
            deque(maxlen=n_media),
            deque(maxlen=n_media),
            deque(maxlen=n_media),
            deque(maxlen=n_media)
    ]

    em_pressao = {}



    # ==================================================
    # ========== Preenche os buffers de dados ==========
    # ==================================================
    # Para garantir funcionamento adequado, o programa espera que os pacotes de dados (buffers) estejam completamente cheios.
    # ENQUANTO não estão cheios, ele continua preenchendo
    print('[PC]: Completando o buffer de dados...')
    while len(buffers[-1]) != n_media:
        saida = esp32.readline().decode('utf-8', errors='ignore').rstrip()

        padrao = r'(T\d+):\s*(\d+)'
        matches = re.findall(padrao, saida)
        saida = [i[1] for i in matches]

        for buffer, valor in enumerate(saida):
            buffers[buffer].append(float(valor)) # "float" pois a leitura retorna palavras ("strings"), não números.
    
    sleep(0.5)
    print('[PC]: Buffer de dados completo.')



    # ==================================================
    # ========== Bloco principal de código ==========
    # ==================================================
    try:
        contador = 0
        while True:
            # ========== Leitura ==========
            saida = esp32.readline().decode('utf-8', errors='ignore').rstrip()
            padrao = r'(T\d+):\s*(\d+)'
            matches = re.findall(padrao, saida)
            saida = [i[1] for i in matches]
            for i, valor in enumerate(saida):
                buffers[i].append(float(valor))


            if contador // n_media == 1: # Faz o loop "while True" ocorrer a cada "n_madia" vezes --> Média de n pontos
                medias = []
                for i in buffers:
                    media = np.array(i).mean()
                    medias.append(media)


                print([float(i) for i in medias]) # Mostra para quem está usando os valores que estão snedo lidos


                precionar_estas_teclas = [] # Guarda as teclas que precisam ser pressionadas. Ex: 't'
                for posicao, valor in enumerate(medias):   

                    # ========== Testa se a medida caiu abaixo de um certo limite ========== 
                    # Se caiu, coloca a tecla para ser apertada
                    # Se não, garnate que ela está "solta"
                    if valor < limiar:
                        precionar_estas_teclas.append(mapa_teclas[f'T{posicao+1}'])
                        em_pressao[posicao] = True

                    else:
                        em_pressao[posicao] = False
                        keyboard.release(mapa_teclas[f'T{posicao+1}'])
                

                # ========== Sequência para apertar todas as teclas listadas ==========
                # Aperta todas as teclas que caíram abaixo do limiar
                for tecla in precionar_estas_teclas:
                    print('[PC]: vou precionar estas:', precionar_estas_teclas)
                    keyboard.press(tecla)
                precionar_estas_teclas = []

                contador = 0 # Reseta o contador para começar uma nova contagem
            contador += 1 # O mesmo que escrever contador = contador + 1


    except KeyboardInterrupt:
        print('[PC]: Encerrando...')
    
    finally:
        esp32.close() # Fecha a comunicação antes de terminar o código por completo





if __name__ == '__main__':

    sequencia_guitar_hero = {
        'T1': 'a',
        'T2': 's',
        'T3': 'j',
        'T4': 'k',
        'T5': 'l',
        'T6': '#'
    }

    sequencia_brilha_brilha = {
        'T1': 'u',
        'T2': '9', 
        'T3': 'y',
        'T4': '6',
        'T5': 'r', 
        'T6': 'e'
    }

    sequencia_jogo = {
        'T1': 'right',
        'T2': 'up', 
        'T3': 'left',
        'T4': 'd',
        'T5': 'w', 
        'T6': 'a'
    }
    rodar(sequencia_guitar_hero, 'COM16', baudrate=9600, n_media=1, limiar=0)