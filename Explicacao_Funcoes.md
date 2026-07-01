# Documentação das Funções - Projeto Damas Brasil

Este documento contém a explicação das principais funções e métodos de todas as classes presentes no código fonte do projeto.

---

## 1. Núcleo do Jogo (`damas_core.hpp` e `damas_core.cpp`)
Estes arquivos contêm a lógica de regras, validação de movimentos e representação do estado do tabuleiro usando Bitboards.

### Funções Globais e Utilitárias
*   **`contar_bits_ativos(BoardBitboard bb)`**: Retorna a quantidade de bits `1` em um bitboard. Utilizado para contar o número de peças.
*   **`init_zobrist_keys()`**: Inicializa as chaves aleatórias de 64 bits necessárias para o cálculo de hashes Zobrist (usado na Tabela de Transposição e na verificação de repetição de posição).

### Classe `BoardState`
Encapsula o estado atual do jogo (posição de peões e damas brancas/pretas, turno, hash, etc).
*   **`configurar_posicao_inicial()`**: Configura o tabuleiro com a organização padrão das 24 peças iniciais do Jogo de Damas.
*   **`definir_posicao_via_bitboards(...)`**: Carrega um estado específico recebendo os bitboards individuais para cada tipo de peça.
*   **`limpar_tabuleiro()`**: Remove todas as peças do tabuleiro e reseta contadores.
*   **`obter_*()`** (ex: `obter_brancas_peoes()`, `obter_turno_atual()`): Métodos getters de acesso de leitura às peças, turno, hash da posição e relógio de empates.
*   **`obter_todas_pecas_brancas() / obter_todas_pecas_pretas() / obter_casas_ocupadas()`**: Geram bitboards combinados (composição) para verificações lógicas rápidas de colisão.
*   **`aplicar_mov_simples(casa_origem, casa_destino)`**: Atualiza as matrizes de bits movendo uma peça para uma casa adjacente. Gerencia também a coroação de damas.
*   **`aplicar_movimento_completo_captura(GameMove)`**: Realiza um salto múltiplo de captura, movendo a peça e removendo as peças inimigas cujos bits correspondam à máscara das capturadas. Zera o contador de meio-movimento.
*   **`alternar_turno()`**: Passa o turno atual de `BRANCAS` para `PRETOS` e vice-versa, além de atualizar a chave Zobrist.
*   **`recalcular_hash_completo()`**: Varre todo o tabuleiro bit a bit gerando o Hash Zobrist do zero (usado em edições manuais no modo montagem).
*   **`definir_* / adicionar_* / remover_*`**: Setters de uso interno para manipular peças individualmente.

### Estruturas de Dados Otimizadas
*   **`GameMove`**: Estrutura que representa um lance (origem, destino e máscara de peças capturadas).
*   **`MoveList`**: Array estático (Stack-allocated) que substitui `std::vector` para armazenar listas de lances gerados sem alocação dinâmica, reduzindo a sobrecarga de memória e otimizando muito a velocidade do motor.

### Geração de Movimentos (Movegen)
*   **`gerar_movimentos_simples_peao(...)`**: Calcula e retorna um bitboard com os destinos válidos (1 casa na diagonal para frente) para um peão.
*   **`gerar_movimentos_simples_dama(...)`**: Calcula e retorna um bitboard com todas as casas disponíveis ao longo das 4 diagonais (raios-X) a partir da posição de uma dama.
*   **`encontrar_sequencias_captura_peao_recursivo(...)`**: Algoritmo recursivo de busca em profundidade que encontra todos os caminhos que um peão pode trilhar para capturar peças (permitindo capturas para trás).
*   **`encontrar_sequencias_captura_dama_recursivo(...)`**: O mesmo que o anterior, porém com as regras de movimento estendido da Dama.
*   **`gerar_todas_capturas_maximais(...)`**: Função principal que usa as duas acima. Ela também **aplica a Lei da Maioria**, descartando rotas que capturem menos peças do que o limite máximo encontrado.

---

## 2. Interface Gráfica (`main.cpp`)
Contém a implementação da interface usando `GTKmm 3.0`. A classe `MainWindow` gere o front-end e liga a visualização ao Core e à IA.

### Inicialização e Construtores
*   **`MainWindow::MainWindow()`**: Monta a janela, invoca os métodos de criação de menu, barra lateral, áreas do tabuleiro e configura sinais (timers e cliques).

### Renderização e Layout
*   **`create_menu_bar() / create_board_area() / create_right_panel()`**: Sub-rotinas organizacionais que constroem instâncias de `Gtk::Box`, `Gtk::DrawingArea` e `Gtk::Stack` respectivamente.
*   **`on_square_draw(cr, row, col)`**: Função executada pelo sinal de renderização (Cairo) que desenha o fundo das casas (clara/escura) e marcações lógicas.
*   **`draw_piece(...)`**: Desenha os gráficos vetoriais das peças e das coroas (no caso de damas) de forma proporcional ao tamanho da casa atual.
*   **`update_board_orientation()`**: Inverte a lógica de desenho das notações (A-H / 1-8) e das posições de matriz para quando o jogador deseja virar o tabuleiro.
*   **`update_ui_dimensions()`**: Recalcula tamanhos das áreas de desenho para suportar janelas dinâmicas (Redimensionar).

### Lógica de Interação
*   **`on_board_click(...)`**: Responde ao clique de mouse, converte pixels em coordenadas lógicas e atua dependendo do modo: modo de jogo (tentar realizar um lance selecionando/movendo) ou modo montagem (adicionar/remover peças).
*   **`show_ambiguous_capture_dialog(...)`**: Pop-up que pergunta ao jogador qual percurso de captura ele deseja seguir quando há múltiplas linhas que terminam na mesma casa final.
*   **`on_key_press_event(...)`**: Captura eventos de teclado para atalhos, como `a, s, d, f, g` para alternar entre os modos de jogo e as setas direcionais `Esquerda/Direita` para Voltar/Avançar no histórico de lances.
*   **`on_setup_board_activated() / draw_palette_item() / on_palette_item_click()`**: Funções responsáveis pelo Modo de Montagem (edição de posições).
*   **`on_undo_activated() / on_redo_activated()`**: Gerencia o voltar/avançar navegando na pilha `m_history_stack`.

### Ciclo de Jogo e IA
*   **`check_game_over_state()`**: Analisa a vitória por extinção de peças inimigas, por falta de movimentos (bloqueio) ou por empates de regra (20 lances de Damas sem captura/avanço de peão, e repetição de posições).
*   **`check_for_ai_move()`**: Dispara o início do processamento da Thread de IA dependendo de quem é o turno e qual o modo de jogo atual (`HUM_VS_HUM`, `HUM_VS_AI`, `AI_VS_AI` ou `ANALYSIS`).
*   **`on_ai_move_ready()`**: Dispatcher acionado quando a IA decide seu lance; aplica ele ao tabuleiro e redesenha a tela.
*   **`on_update_analysis_timer()`**: Timer periódico que atualiza a GUI com pontuação, nós analisados e profundidade da IA no painel lateral em tempo real.
*   **Gestão de Nível/Tempo**: O tempo limite da IA é controlado de forma ajustável via menu (variando de 1 segundo a 60 segundos por lance), passado para o motor via a estrutura `Search_Input`.
*   **Menu Aprender / `aprender_com_correcao`**: A interface captura o estado anterior a um "Undo" (Voltar) e o compara com o novo lance correto ensinado pelo usuário, invocando a rotina de aprendizado de máquina no motor.

### Gestão de Arquivos (FEN / PDN)
*   **`generateFEN() / setBoardFromFEN()`**: Serializa/Dessorieliza o estado atual em uma representação alfanumérica string.
*   **`saveFEN() / loadFEN() / savePDN() / loadPDN() / importBatchFile()`**: Manipulação do `FileChooserDialog` para abertura e persistência física de arquivos `.fen`, partidas `.pdn` e processamento em lote.
*   **`showBatchSelectionDialog()`**: Interface de lista (TreeView) que permite visualizar e escolher instantaneamente qual FEN carregar a partir de um arquivo em lote (`.txt` ou `.pdn`) importado.

---

## 3. Inteligência Artificial (`damas_ai.hpp` e `damas_ai.cpp`)
Implementa o Alpha-Beta Pruning, Tabelas de Transposição (Hash) e o Tablebase Probe de Finais (EGTB).

### Motor Global
*   **`search(so, pos, input)`**: A API global de entrada da IA. Reseta os atributos, recarrega os pesos de aprendizado e inicia a iteração. Intercepta imediatamente lances na raiz se encontrar um "gabarito" já ensinado pelo usuário.
*   **`interrupt_search() / clear_interrupt() / get_current_so() / clear_hash()`**: Utilitários para controle das threads em background e extração do melhor lance calculado até o momento.
*   **`aprender_com_correcao(estado_bom, estado_ruim)`**: Avalia e compara dois estados pós-lance. Modula os pesos das 18 heurísticas pelo Gradiente de Erro (Reforço) e salva os hashes Zobrist na Memória Fotográfica (`padroes_bons.bin` e `padroes_ruins.bin`). Executa o espelhamento do tabuleiro bit a bit, garantindo que o aprendizado seja válido imediatamente tanto para as Brancas quanto para as Pretas.
*   **`carregar_pesos_ia() / carregar_padroes_memoria()`**: Rotinas que carregam as memórias fotográficas e heurísticas. Contam com um sistema de migração automática que converte os antigos dados em `.txt` para os formatos otimizados em `.bin`.

### Classe `PatriotasEngine`
*   **`evaluate(BoardState)`**: Função heurística avaliadora estática. Analisa os 18 parâmetros posicionais do tabuleiro (Ex: Domínio do Carreirão, Defesa de Base, Pedras Cão), multiplicados dinamicamente pelos pesos mutáveis. Também aplica punições e bonificações esmagadoras se a posição combinar com algo na Memória Fotográfica.
*   **`quiescence(pos, alpha, beta)`**: A Busca de Aquiescência. Garante que as sequências de capturas obrigatórias se resolvam antes da avaliação final do nó para não sofrer o "efeito do horizonte". Também conta com bloqueio imediato contra anti-padrões fotográficos.
*   **`alpha_beta(pos, depth, alpha, beta, ply, do_null)`**: O cérebro da busca. Implementa recursivamente o Principal Variation Search, uso de History Heuristics e podas com verificação de tabela Hash e EGTB. Descarta precocemente ramificações que levem a anti-padrões catalogados.
*   **`sort_moves(...)`**: Ordena a lista de possíveis movimentos para aumentar a taxa de corte Alfa-Beta (lances bons primeiro).
*   **`extract_pv(...)`**: Extrai a Linha Principal (Principal Variation) do histórico de hashes calculados e retorna a sequência esperada.

### Classe `EGTB`
*   **`probe(pos, dtm)`**: Acessa e lê os bancos de dados pré-calculados `.lbnz` (Endgame Tablebases). Contém uma regra integrada para evitar vazamento de memória RAM, limpando o cache automaticamente quando atinge 8 tabelas de tamanho máximo.
*   **`probe_with_move(pos, best_move, score, dtm)`**: Verifica não apenas quem ganha a posição pelo banco de dados, mas procura todas as sucessoras diretas para decidir o lance exato em menor número de movimentos em posição de até 6 peças.

---

## 4. Rede e Integração TCP DXP (`damas_dxp.hpp` e `damas_dxp.cpp`)
Implementa o protocolo de conexão a servidores DXP (DamExchange Protocol).

### Classe `DamasDXP`
*   **`conectar(host, port) / desconectar()`**: Cria um socket TCP bloqueante e dispara a thread `loop_escuta`.
*   **`enviar_mensagem(...)`**: Acrescenta `\0` para compatibilidade final com o socket e remete strings textuais.
*   **`enviar_game_req(...) / enviar_game_acc(...) / enviar_movimento(...) / enviar_game_end(...)`**: Constroem programaticamente strings conformes ao padrão de caracteres e comprimentos DXP (como 'R', 'A', 'M', 'E').
*   **`loop_escuta()`**: Função que funciona via Thread esperando inputs infinitos. Quando recebe, quebra-os pelos delimitadores e joga em buffers para o Dispatcher analisar.
*   **`processar_mensagem_dxp(msg)`**: Recebe a string montada e invoca de volta as Callbacks de alto nível na interface.
*   **`atualizar_pesos_aprendizado(resultado)`**: Realiza o Reinforcement Learning. Aplica uma trava (File Lock com `flock()`) que garante o acesso exclusivo em disco se múltiplas IAs estiverem rodando juntas. Grava as novas mutações genéticas no formato binário em `pesos_tabela.bin` e faz fallback da melhor memória segura em `pesos_memoria.bin` em caso de derrota.
*   **`Scanner`**: Utilitário em classe que extrai substrings por tamanho, já que os protocolos legados como DXP usam larguras fixas por colunas.

---
