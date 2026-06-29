/*
 * Patriotas - Motor e Interface para Jogo de Damas Brasileiro
 * Copyright (C) 2024 Emanoel Libonati
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <gtkmm.h>
#include <iostream>
#include <vector>
#include <cmath> // Para std::round
#include <sstream> // Para std::stringstream
#include <iomanip> // Para std::setprecision
#include <thread>  // Para std::thread
#include <atomic>
#include <future>  // Para std::future e std::async
#include <glibmm/main.h> // Para Glib::signal_idle
#include <filesystem>
#include <fstream>
#include <ctime>
#include <cctype>
#include <cstdlib> // Para std::system
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/filefilter.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treeview.h>
#include "damas_dxp.hpp" // Adiciona integração DXP
#include "damas_core.hpp" // Nova lógica do jogo
#include "damas_ai.hpp"   // Motor Patriotas

// Removido SQUARE_SIZE global

class MainWindow : public Gtk::Window
{
public:
    MainWindow();
    virtual ~MainWindow();

    // Define PaletteTool enum aqui, para que seja visível antes de ser usada
    enum class PaletteTool {
        NONE,
        WHITE_PAWN,
        BLACK_PAWN,
        WHITE_KING,
        BLACK_KING,
        ERASER
    };

    // Struct para armazenar o estado do jogo e a notação do movimento no histórico
    struct HistoryEntry {
        DamasCore::BoardState state;
        std::string move_notation; // Vazio para o estado inicial
        int last_move_from_sq;
        int last_move_to_sq;
    };

protected:
    // Manipulador de sinal para desenhar as casas do tabuleiro
    bool on_square_draw(const Cairo::RefPtr<Cairo::Context>& cr, int row, int col);
    bool on_update_analysis_timer(); // Handler para o timer de análise da IA
    bool on_board_click(GdkEventButton* event, int row, int col);
    void on_connect_activated(); // Handlers de rede
    void on_disconnect_activated();
    void show_game_over_dialog(const std::string& message);
    bool check_game_over_state(); // Verifica vitória/empate
    void on_ai_move_ready(); // Handler para quando a IA terminar
    void draw_piece(const Cairo::RefPtr<Cairo::Context>& cr, DamasCore::GamePlayer player, bool is_king, double piece_size);

    // Funções para lidar com capturas ambíguas
    void show_ambiguous_capture_dialog(const DamasCore::GameMove* moves_ptr, size_t count);
    std::string get_captured_squares_string(DamasCore::BoardBitboard captured_mask);
    // Funções auxiliares para criar os componentes da UI
    void create_menu_bar();
    Gtk::Widget* create_board_area(); // Retorna Gtk::Alignment*
    Gtk::Stack* create_right_panel(); // Retorna Gtk::Stack*

    // Funções de Debug
    Gtk::CheckMenuItem* m_debug_estado_item = nullptr;
    Gtk::CheckMenuItem* m_debug_arvore_item = nullptr;
    Gtk::CheckMenuItem* m_debug_avaliar_item = nullptr;
    void print_board_state();
    void print_evaluation();
    void log_debug_info();

    // Helper para atualizar a orientação do tabuleiro (girar)
    void update_board_orientation();

    // Helper para atualizar as dimensões da UI (tamanho das casas, posições dos divisores, tamanho da janela)
    void update_ui_dimensions(int new_square_size, int new_rule_padding, int new_window_width, int new_window_height);


    // Widgets principais
    Gtk::Box m_main_vbox;
    Gtk::MenuBar m_menu_bar;
    Gtk::Box m_h_box; // Usa Gtk::Box para o layout horizontal, para melhor controle de expansão
    Gtk::Stack m_right_panel_stack; // Usaremos Gtk::Stack para alternar entre o painel normal e a paleta
    Gtk::Box* m_v_paned_right; // Divisor vertical (análise | histórico) - Alterado para Gtk::Box

    // Widgets do tabuleiro
    // Vetor para armazenar ponteiros para as DrawingArea das casas do tabuleiro,
    // permitindo que seus tamanhos sejam atualizados dinamicamente.
    Gtk::DrawingArea* m_board_squares[8][8]{};
    Gtk::Grid m_board_grid;
    // Vetores para armazenar ponteiros para os labels das réguas
    Gtk::Label* m_top_ruler_labels[8]{};
    Gtk::Label* m_bottom_ruler_labels[8]{};
    Gtk::Label* m_left_ruler_labels[8]{};
    Gtk::Label* m_right_ruler_labels[8]{};

    // Widgets do painel direito
    Gtk::Frame m_analysis_frame;
    Gtk::Box* m_analysis_box = nullptr;
    Gtk::Label* m_pv_title_label = nullptr;
    Gtk::TextView* m_pv_textview = nullptr;
    Glib::RefPtr<Gtk::TextBuffer> m_pv_text_buffer;
    Gtk::Label* m_depth_label_value = nullptr;
    Gtk::Label* m_nodes_label_value = nullptr; 
    Gtk::Label* m_score_label_value = nullptr;
    Gtk::Label* m_time_label_value = nullptr;
    Gtk::Frame m_history_frame;
    Gtk::ScrolledWindow m_history_scrolled_window;
    Gtk::TextView m_history_text_view;

    // Estado da UI
    bool m_is_large = false; // Para rastrear o estado do tamanho da janela
    bool m_is_flipped = false; // Para rastrear a orientação do tabuleiro
    int m_current_square_size; // Tamanho atual de uma casa do tabuleiro
    int m_current_rule_padding; // Preenchimento atual para as réguas no divisor horizontal
    double m_analysis_panel_ratio; // Proporção para o painel vertical direito (análise/histórico)

    // --- Modo de Jogo ---
    enum class GameMode {
        HUM_VS_HUM,
        HUM_VS_AI,
        AI_VS_HUM,
        AI_VS_AI,
        ANALYSIS
    };
    GameMode m_game_mode = GameMode::HUM_VS_HUM; // Default to Human vs Human
    void set_game_mode(GameMode mode);
    void set_menus_sensitive(bool sensitive);
    void check_for_ai_move();

    // --- Estado do Jogo ---
    DamasCore::BoardState m_game_state;
    int m_selected_square = -1; // -1 indica nenhuma peça selecionada (índice 0-63)
    DamasCore::BoardBitboard m_possible_moves_bitboard = 0; // Bitboard de casas de destino para movimentos simples
    DamasCore::MoveList m_possible_capture_moves; // Lista de movimentos de captura maximais na memória stack
    DamasCore::BoardBitboard m_mandatory_capture_origins = 0; // Bitboard das peças que devem fazer uma captura

    // Setup Mode related members
    bool m_in_setup_mode = false;
    Gtk::Box* m_palette_vbox; // The main container for the palette - Agora é um ponteiro
    Gtk::DrawingArea m_white_pawn_palette_item, m_black_pawn_palette_item, m_white_king_palette_item, m_black_king_palette_item, m_eraser_palette_item;
    Gtk::Button m_setup_clear_initial_button, m_setup_done_button;
    Gtk::RadioButton m_setup_white_turn_button, m_setup_black_turn_button; // Renomeado para evitar conflito
    DamasCore::GamePlayer m_setup_turn = DamasCore::GamePlayer::BRANCAS; // To set the turn in setup mode
    PaletteTool m_selected_palette_tool = PaletteTool::NONE;

    // Nova função para montar o tabuleiro
    void on_setup_board_activated();

    // Setup mode specific handlers
    bool draw_palette_item(const Cairo::RefPtr<Cairo::Context>& cr, PaletteTool tool);
    bool on_palette_item_click(GdkEventButton* event, PaletteTool tool);
    void on_setup_clear_initial_clicked();
    void on_setup_white_turn_clicked();
    void on_setup_black_turn_clicked();
    void on_setup_done_clicked();
    void reset_game_history(const DamasCore::BoardState& initial_state, const std::string& initial_notation);
    void set_palette_tool(PaletteTool tool);
    std::string get_algebraic_notation(int from_sq, int to_sq, DamasCore::BoardBitboard captured_mask);

    // Handlers for Undo/Redo
    void on_undo_activated();
    void on_redo_activated();
    void update_undo_redo_sensitivity();

    // Novo: record_new_state agora aceita a notação do movimento
    void record_new_state(const std::string& move_notation);
    void update_history_text_view();

    // Função para tocar som de movimento
    void play_move_sound();

    // Keyboard event handler
    bool on_key_press_event(GdkEventKey* key_event) override;

    bool m_is_board_cleared = false; // Novo membro para rastrear o estado do tabuleiro
    // History for Undo/Redo
    HistoryEntry m_history_stack[2048];
    DamasCore::BoardState m_last_undone_state; // Guarda memória do lance ruim para aprendizado
    std::vector<DamasCore::GameMove> m_saved_win_pv; // Memória da sequência de lances do ganho
    size_t m_history_count = 0;
    // Variáveis para rastrear o último movimento para o rastro visual
    int m_last_move_from_sq = -1;
    int m_last_move_to_sq = -1;

    size_t m_history_index = 0;

    // Pointers to menu items for sensitivity control
    Gtk::MenuItem* m_undo_item = nullptr;
    Gtk::MenuItem* m_redo_item = nullptr;
    Gtk::MenuItem* m_arquivo_menu_item = nullptr;
    Gtk::MenuItem* m_mode_menu_item = nullptr;
    Gtk::MenuItem* m_level_menu_item = nullptr;
    Gtk::MenuItem* m_rede_menu_item = nullptr;
    Gtk::MenuItem* m_aprender_menu_item = nullptr;
    Gtk::MenuItem* m_resize_item = nullptr;
    Gtk::MenuItem* m_flip_item = nullptr;
    Gtk::MenuItem* m_setup_menu_item = nullptr;

    Gtk::MenuItem* m_connect_item = nullptr;
    Gtk::MenuItem* m_disconnect_item = nullptr;
    DamasCore::DamasDXP m_dxp_client; // Novo objeto do cliente de rede

    // Status de Rede na Barra de Menu
    int m_network_game_state = 0; // 0 = offline, 1 = aguardando, 2 = jogando
    Gtk::MenuItem* m_network_status_item = nullptr;
    Gtk::Label* m_network_status_label = nullptr;
    void update_network_status();

    // Ponteiros para os itens do menu de modo para ativação por atalho
    Gtk::RadioMenuItem* m_hum_vs_hum_item = nullptr;
    Gtk::RadioMenuItem* m_hum_vs_ai_item = nullptr;
    Gtk::RadioMenuItem* m_ai_vs_hum_item = nullptr;
    Gtk::RadioMenuItem* m_ai_vs_ai_item = nullptr;
    Gtk::RadioMenuItem* m_analysis_item = nullptr;

    DamasCore::GamePlayer m_minha_cor_dxp = DamasCore::GamePlayer::BRANCAS;

    // --- IA ---
    double m_ai_time_limit = 1.0;
    Glib::Dispatcher m_ai_move_dispatcher;
    DamasCore::Search_Output m_ai_result_so;
    std::thread m_ai_thread;
    std::atomic<bool> m_is_ai_turn_active {false};
    bool m_discard_next_ai_move {false};
    sigc::connection m_ai_chain_timer_conn;
private:
    std::string format_time(double total_seconds);
    std::string format_pv(const std::vector<DamasCore::GameMove>& pv, DamasCore::BoardState initial_state);
    std::string format_nodes(uint64_t nodes);
  // Métodos para Salvar/Abrir (FEN/PDN)
    std::string runFileDialog(const Glib::ustring& title, Gtk::FileChooserAction action);
    std::string generateFEN();
    void setBoardFromFEN(const std::string& fen);
    void saveFEN(const std::string& filename);
    void loadFEN(const std::string& filename);
    void savePDN(const std::string& filename, bool saveHistory);
    void loadPDN(const std::string& filename);
    bool applyMoveStr(const std::string& moveStr);
    void importBatchFile(const std::string& filename);
    void showBatchSelectionDialog();
    std::string getCurrentDate();

    // Estrutura das colunas para a lista de Lote
    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns() { add(m_col_index); add(m_col_fen); }
        Gtk::TreeModelColumn<int> m_col_index;
        Gtk::TreeModelColumn<Glib::ustring> m_col_fen;
    };
    ModelColumns m_batchColumns;
    std::vector<DamasCore::BoardState> m_batch_positions;
    std::vector<std::string> m_batch_labels;
};
MainWindow::MainWindow() : 
    m_main_vbox(Gtk::ORIENTATION_VERTICAL, 0),
    m_h_box(Gtk::ORIENTATION_HORIZONTAL, 5),
    m_analysis_frame("Análise"),
    m_history_frame("Histórico"),
    m_current_square_size(60), // Tamanho inicial pequeno
    m_current_rule_padding(60), // Preenchimento inicial pequeno para as réguas
    m_right_panel_stack(), // Inicializa o membro Gtk::Stack
    m_analysis_panel_ratio(0.4), // Proporção inicial para o painel de análise
    // Inicializa os botões membros da paleta
    m_setup_clear_initial_button("Limpar/Inicial"),
    m_setup_done_button("Pronto"),
    m_setup_white_turn_button("Brancas"),
    m_setup_black_turn_button("Pretas")
{
    // Inicializa as chaves Zobrist uma única vez no início do programa
    DamasCore::init_zobrist_keys();

    set_title("PATRIOTAS");
    try {
        std::error_code ec_sym;
        std::filesystem::path exe_dir = std::filesystem::read_symlink("/proc/self/exe", ec_sym).parent_path();
        if (!ec_sym && std::filesystem::exists(exe_dir / "img" / "patriotas.png")) {
            set_icon_from_file((exe_dir / "img" / "patriotas.png").string());
        } else {
            set_icon_from_file("img/patriotas.png");
        }
    } catch (...) {
        try {
            set_icon_from_file("../img/patriotas.png");
        } catch (...) {
            std::cerr << "Aviso: Nao foi possivel carregar o icone patriotas.png" << std::endl;
        }
    }
    set_resizable(false);
    set_border_width(5);

    // Habilita a janela a receber eventos de teclado
    add_events(Gdk::KEY_PRESS_MASK);

    // Configura a posição inicial do jogo
    m_game_state.configurar_posicao_inicial();

    // Inicializa o histórico de lances com a posição inicial
    m_history_stack[0] = {m_game_state, "", -1, -1}; // Estado inicial sem movimento, sem rastro
    m_history_count = 1;
    m_history_index = 0;

    // Inicializa os painéis que agora são ponteiros
    m_v_paned_right = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 5);
    m_palette_vbox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL);

    // Conecta o dispatcher da IA ao seu handler no thread principal
    m_ai_move_dispatcher.connect(sigc::mem_fun(*this, &MainWindow::on_ai_move_ready));

    // Conecta o timer de atualização da análise (10 vezes por segundo)
    Glib::signal_timeout().connect(sigc::mem_fun(*this, &MainWindow::on_update_analysis_timer), 100);

    // 1. Adiciona a caixa vertical principal à janela.
    add(m_main_vbox);

    // 2. Cria e adiciona a barra de menus no topo.
    create_menu_bar();
    m_main_vbox.pack_start(m_menu_bar, Gtk::PACK_SHRINK);

    // 3. Adiciona o painel divisor horizontal principal.
    m_main_vbox.pack_start(m_h_box, Gtk::PACK_EXPAND_WIDGET);

    // 4. Cria a área do tabuleiro e o painel direito.
    Gtk::Widget* board_area = create_board_area();
    Gtk::Stack* right_panel_stack_ptr = create_right_panel(); // create_right_panel agora configura m_right_panel_stack

    // 5. Adiciona o tabuleiro à esquerda e o painel à direita do divisor.
    m_h_box.pack_start(*board_area, Gtk::PACK_SHRINK);
    m_h_box.pack_start(*right_panel_stack_ptr, Gtk::PACK_EXPAND_WIDGET);

    // Define a posição inicial do divisor principal para dar espaço ao tabuleiro, usando as variáveis de membro.
    // Define o tamanho inicial da janela após o layout inicial
    set_default_size(m_current_square_size * 8 + m_current_rule_padding + 150, m_current_square_size * 8 + m_current_rule_padding);

    update_history_text_view(); // Popula o histórico inicial
    // Exibe todos os widgets filhos.
    show_all_children();

    // Define a sensibilidade inicial dos botões de undo/redo
    update_undo_redo_sensitivity();
}

MainWindow::~MainWindow()
{
    // Garante que o thread da IA termine antes de a janela ser destruída
    DamasCore::interrupt_search();
    if (m_ai_thread.joinable()) {
        m_ai_thread.join();
    }
}

void MainWindow::create_menu_bar()
{
    // --- Menu "Arquivo" ---
    m_arquivo_menu_item = Gtk::make_managed<Gtk::MenuItem>("_Arquivo", true);
    auto arquivo_menu = Gtk::make_managed<Gtk::Menu>();
    m_arquivo_menu_item->set_submenu(*arquivo_menu);

    auto new_game_item = Gtk::make_managed<Gtk::MenuItem>("_Novo Jogo", true);
    new_game_item->signal_activate().connect([this]() {
        if (m_is_ai_turn_active) {
            m_discard_next_ai_move = true;
            DamasCore::interrupt_search();
        }
        m_game_mode = GameMode::HUM_VS_HUM;
        DamasCore::BoardState initial_state; initial_state.configurar_posicao_inicial();
        reset_game_history(initial_state, "");
    });
    arquivo_menu->append(*new_game_item);

    // --- Novo Menu "Modo" ---
    m_mode_menu_item = Gtk::make_managed<Gtk::MenuItem>("_Modo", true);
    auto mode_menu = Gtk::make_managed<Gtk::Menu>();
    m_mode_menu_item->set_submenu(*mode_menu);

    Gtk::RadioMenuItem::Group mode_group;

    m_hum_vs_hum_item = Gtk::make_managed<Gtk::RadioMenuItem>(mode_group);
    auto hum_vs_hum_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 10);
    hum_vs_hum_box->pack_start(*Gtk::make_managed<Gtk::Label>("Hum vs Hum"), Gtk::PACK_SHRINK); hum_vs_hum_box->pack_end(*Gtk::make_managed<Gtk::Label>("a"), Gtk::PACK_SHRINK);
    m_hum_vs_hum_item->add(*hum_vs_hum_box);
    m_hum_vs_hum_item->signal_activate().connect([this]() { set_game_mode(GameMode::HUM_VS_HUM); });
    mode_menu->append(*m_hum_vs_hum_item);

    m_hum_vs_ai_item = Gtk::make_managed<Gtk::RadioMenuItem>(mode_group);
    auto hum_vs_ai_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 10);
    hum_vs_ai_box->pack_start(*Gtk::make_managed<Gtk::Label>("Hum vs IA"), Gtk::PACK_SHRINK);
    hum_vs_ai_box->pack_end(*Gtk::make_managed<Gtk::Label>("s"), Gtk::PACK_SHRINK);
    m_hum_vs_ai_item->add(*hum_vs_ai_box);
    m_hum_vs_ai_item->signal_activate().connect([this]() { set_game_mode(GameMode::HUM_VS_AI); });
    mode_menu->append(*m_hum_vs_ai_item);

    m_ai_vs_hum_item = Gtk::make_managed<Gtk::RadioMenuItem>(mode_group);
    auto ai_vs_hum_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 10);
    ai_vs_hum_box->pack_start(*Gtk::make_managed<Gtk::Label>("IA vs Hum"), Gtk::PACK_SHRINK);
    ai_vs_hum_box->pack_end(*Gtk::make_managed<Gtk::Label>("d"), Gtk::PACK_SHRINK);
    m_ai_vs_hum_item->add(*ai_vs_hum_box);
    m_ai_vs_hum_item->signal_activate().connect([this]() { set_game_mode(GameMode::AI_VS_HUM); });
    mode_menu->append(*m_ai_vs_hum_item);

    m_ai_vs_ai_item = Gtk::make_managed<Gtk::RadioMenuItem>(mode_group);
    auto ai_vs_ai_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 10);
    ai_vs_ai_box->pack_start(*Gtk::make_managed<Gtk::Label>("IA vs IA"), Gtk::PACK_SHRINK);
    ai_vs_ai_box->pack_end(*Gtk::make_managed<Gtk::Label>("f"), Gtk::PACK_SHRINK);
    m_ai_vs_ai_item->add(*ai_vs_ai_box);
    m_ai_vs_ai_item->signal_activate().connect([this]() { set_game_mode(GameMode::AI_VS_AI); });
    mode_menu->append(*m_ai_vs_ai_item);

    mode_menu->append(*Gtk::make_managed<Gtk::SeparatorMenuItem>());

    m_analysis_item = Gtk::make_managed<Gtk::RadioMenuItem>(mode_group);
    auto analysis_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 10);
    analysis_box->pack_start(*Gtk::make_managed<Gtk::Label>("Análise"), Gtk::PACK_SHRINK);
    analysis_box->pack_end(*Gtk::make_managed<Gtk::Label>("g"), Gtk::PACK_SHRINK);
    m_analysis_item->add(*analysis_box);
    m_analysis_item->signal_activate().connect([this]() { set_game_mode(GameMode::ANALYSIS); });
    mode_menu->append(*m_analysis_item);

    // --- Novo Menu "Nível" ---
    m_level_menu_item = Gtk::make_managed<Gtk::MenuItem>("_Nível", true);
    auto level_menu = Gtk::make_managed<Gtk::Menu>();
    m_level_menu_item->set_submenu(*level_menu);

    Gtk::RadioMenuItem::Group level_group;
    const int time_options[] = {1, 3, 5, 10, 15, 20, 30, 60};

    for (int time_sec : time_options) {
        auto item = Gtk::make_managed<Gtk::RadioMenuItem>(level_group, std::to_string(time_sec) + " seg");
        if (time_sec == 1) item->set_active(true);
        item->signal_toggled().connect([this, time_sec, item]() {
            if (item->get_active()) m_ai_time_limit = time_sec;
        });
        level_menu->append(*item);
    }

    m_hum_vs_hum_item->set_active(true); // Define Hum vs Hum como modo padrão

    arquivo_menu->append(*Gtk::make_managed<Gtk::SeparatorMenuItem>());

    auto open_game_item = Gtk::make_managed<Gtk::MenuItem>("Abrir Partida");
    open_game_item->signal_activate().connect([this]() {
        if (m_is_ai_turn_active) {
            m_discard_next_ai_move = true;
            DamasCore::interrupt_search();
        }
        std::string path = runFileDialog("Abrir Partida (.pdn)", Gtk::FILE_CHOOSER_ACTION_OPEN);
        if (!path.empty()) {
            loadPDN(path);
        }
    });
    arquivo_menu->append(*open_game_item);

    auto save_game_item = Gtk::make_managed<Gtk::MenuItem>("Salvar Partida");
    save_game_item->signal_activate().connect([this]() {
        std::string path = runFileDialog("Salvar Partida (.pdn)", Gtk::FILE_CHOOSER_ACTION_SAVE);
        if (!path.empty()) {
            if (path.find(".pdn") == std::string::npos) path += ".pdn";
            savePDN(path, true);
        }
    });
    arquivo_menu->append(*save_game_item);

    arquivo_menu->append(*Gtk::make_managed<Gtk::SeparatorMenuItem>());

    auto open_pos_item = Gtk::make_managed<Gtk::MenuItem>("Abrir Posição");
    open_pos_item->signal_activate().connect([this]() {
        if (m_is_ai_turn_active) {
            m_discard_next_ai_move = true;
            DamasCore::interrupt_search();
        }
        std::string path = runFileDialog("Abrir Posição (.fen)", Gtk::FILE_CHOOSER_ACTION_OPEN);
        if (!path.empty()) {
            loadFEN(path);
        }
    });
    arquivo_menu->append(*open_pos_item);

    auto save_pos_item = Gtk::make_managed<Gtk::MenuItem>("Salvar Posição");
    save_pos_item->signal_activate().connect([this]() {
        std::string path = runFileDialog("Salvar Posição (.fen)", Gtk::FILE_CHOOSER_ACTION_SAVE);
        if (!path.empty()) {
            if (path.find(".fen") == std::string::npos) path += ".fen";
            saveFEN(path);
        }
    });
    arquivo_menu->append(*save_pos_item);

    arquivo_menu->append(*Gtk::make_managed<Gtk::SeparatorMenuItem>());

    auto open_batch_item = Gtk::make_managed<Gtk::MenuItem>("Abrir em Lote");
    open_batch_item->signal_activate().connect([this]() {
        if (m_is_ai_turn_active) {
            m_discard_next_ai_move = true;
            DamasCore::interrupt_search();
        }
        std::string path = runFileDialog("Abrir em Lote (.txt, .fen, .pdn)", Gtk::FILE_CHOOSER_ACTION_OPEN);
        if (!path.empty()) {
            importBatchFile(path);
            if (!m_batch_positions.empty()) {
                showBatchSelectionDialog();
            } else {
                Gtk::MessageDialog dialog(*this, "Aviso", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
                dialog.set_secondary_text("Nenhuma posição FEN válida encontrada no arquivo.");
                dialog.run();
            }
        }
    });
    arquivo_menu->append(*open_batch_item);

    auto quit_item = Gtk::make_managed<Gtk::MenuItem>("_Sair", true);
    quit_item->signal_activate().connect([this]() {
        hide(); // Fecha a janela principal
    });
    arquivo_menu->append(*quit_item);

    // --- Menu "Ajuda" ---
    auto help_menu_item = Gtk::make_managed<Gtk::MenuItem>("_Ajuda", true);
    auto help_menu = Gtk::make_managed<Gtk::Menu>();
    help_menu_item->set_submenu(*help_menu);

    Glib::RefPtr<Gdk::Pixbuf> app_logo;
    try {
        std::error_code ec_sym;
        std::filesystem::path exe_dir = std::filesystem::read_symlink("/proc/self/exe", ec_sym).parent_path();
        if (!ec_sym && std::filesystem::exists(exe_dir / "img" / "patriotas.png")) {
            app_logo = Gdk::Pixbuf::create_from_file((exe_dir / "img" / "patriotas.png").string());
        } else {
            app_logo = Gdk::Pixbuf::create_from_file("img/patriotas.png");
        }
    } catch (...) {
        try {
            app_logo = Gdk::Pixbuf::create_from_file("../img/patriotas.png");
        } catch (...) {}
    }

    auto how_to_use_item = Gtk::make_managed<Gtk::MenuItem>("Como Usar", true);
    if (auto label = dynamic_cast<Gtk::Label*>(how_to_use_item->get_child())) label->set_halign(Gtk::ALIGN_START);
    how_to_use_item->signal_activate().connect([this, app_logo]() {
        Gtk::MessageDialog dialog(*this, "Como usar o Patriotas", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
        if (app_logo) dialog.set_icon(app_logo);
        dialog.set_secondary_text(
            "Bem-vindo ao Patriotas!\n\n"
            "• Para jogar contra a IA, selecione 'Modo -> Hum vs IA'.\n"
            "• Ajuste o tempo de reflexão da IA no menu 'Nível'.\n"
            "• Use 'Tabuleiro -> Montar' para criar posições personalizadas.\n"
            "• Jogue online através do menu 'Rede -> Conectar' (DXP).\n"
            "• Ative o modo de 'Análise' para ver a avaliação em tempo real."
        );
        dialog.run();
    });
    help_menu->append(*how_to_use_item);

    auto features_donate_item = Gtk::make_managed<Gtk::MenuItem>("Recursos e Doação", true);
    if (auto label = dynamic_cast<Gtk::Label*>(features_donate_item->get_child())) label->set_halign(Gtk::ALIGN_START);
    features_donate_item->signal_activate().connect([this, app_logo]() {
        Gtk::MessageDialog dialog(*this, "Recursos e Apoio", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
        if (app_logo) dialog.set_icon(app_logo);
        dialog.set_secondary_text(
            "O Patriotas é um motor moderno de Jogo de Damas Brasileiro com:\n"
            "• Inteligência Artificial Avançada (Alpha-Beta + TT)\n"
            "• Integração nativa com Base de Finais (EGTB)\n"
            "• Partidas em rede via Protocolo DXP\n"
            "• Ferramentas de análise avançada de FEN e PDN\n\n"
            "Se você gosta deste projeto e deseja apoiar o desenvolvimento com qualquer valor, utilize a chave PIX:\n\n"
            "PIX (E-mail): brasillinux20@gmail.com\n\n"
            "Muito obrigado pelo seu apoio!"
        );
        dialog.run();
    });
    help_menu->append(*features_donate_item);

    auto compat_item = Gtk::make_managed<Gtk::MenuItem>("Compatibilidade", true);
    if (auto label = dynamic_cast<Gtk::Label*>(compat_item->get_child())) label->set_halign(Gtk::ALIGN_START);
    compat_item->signal_activate().connect([this, app_logo]() {
        Gtk::MessageDialog dialog(*this, "Aviso de Compatibilidade", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
        if (app_logo) dialog.set_icon(app_logo);
        dialog.set_secondary_text(
            "Este programa atende exclusivamente ao GNU/Linux Debian e seus derivados (como MX Linux, Ubuntu, Linux Mint, Pop!_OS, etc.)."
        );
        dialog.run();
    });
    help_menu->append(*compat_item);

    help_menu->append(*Gtk::make_managed<Gtk::SeparatorMenuItem>());

    auto about_item = Gtk::make_managed<Gtk::MenuItem>("_Sobre", true);
    if (auto label = dynamic_cast<Gtk::Label*>(about_item->get_child())) label->set_halign(Gtk::ALIGN_START);
    about_item->signal_activate().connect([this, app_logo]() {
        Gtk::AboutDialog dialog;
        dialog.set_transient_for(*this);
        if (app_logo) {
            dialog.set_icon(app_logo);
            dialog.set_logo(app_logo->scale_simple(100, 100, Gdk::INTERP_BILINEAR));
        }
        dialog.set_program_name("Patriotas");
        dialog.set_version("1.0");
        dialog.set_copyright(u8"© 2024 Emanoel Libonati");
        dialog.set_authors({"Emanoel Libonati"});
        dialog.set_comments(
            "Interface para jogo de damas brasileiro.\n\n"
            "Este é um software livre sob a licença GNU GPL, garantindo as 4 liberdades essenciais:\n"
            "0. A liberdade de executar o programa como você desejar, para qualquer propósito.\n"
            "1. A liberdade de estudar como o programa funciona e adaptá-lo às suas necessidades.\n"
            "2. A liberdade de redistribuir cópias para que você possa ajudar ao próximo.\n"
            "3. A liberdade de melhorar o programa e lançar suas melhorias para o público."
        );
        dialog.set_license_type(Gtk::LICENSE_GPL_3_0);
        dialog.run();
    });
    help_menu->append(*about_item);

    // --- Novo Menu "Aprender" ---
    m_aprender_menu_item = Gtk::make_managed<Gtk::MenuItem>("_Aprender", true);
    auto aprender_menu = Gtk::make_managed<Gtk::Menu>();
    m_aprender_menu_item->set_submenu(*aprender_menu);

    auto corrigir_item = Gtk::make_managed<Gtk::MenuItem>("Aprender com Correção", true);
    corrigir_item->signal_activate().connect([this]() {
        if (m_last_undone_state.obter_casas_ocupadas() != 0) {
            DamasCore::aprender_com_correcao(m_game_state, m_last_undone_state);
            DamasCore::clear_hash(); // Esvazia a RAM para a IA não reaproveitar cálculos errados de antes
            
            Gtk::MessageDialog dialog(*this, "Aprendizado Concluído", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
            dialog.set_secondary_text("A IA comparou seu lance correto com o lance ruim que ela fez.\n\n1. Os pesos heurísticos foram modulados usando Gradiente de Erro para evitar esse raciocínio no futuro.\n2. A posição ruim exata virou um novo parâmetro (Anti-Padrão fotográfico) para bloquear reincidência total.");
            dialog.run();

            m_last_undone_state.limpar_tabuleiro(); // Reseta para não aplicar a mesma lição duas vezes
        } else {
            Gtk::MessageDialog dialog(*this, "Nenhuma Correção Detectada", false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
            dialog.set_secondary_text("Para treinar a IA, siga os passos:\n\n1. Deixe ela jogar um lance ruim.\n2. Clique em Tabuleiro -> 'Voltar' (<<).\n3. Jogue o lance correto no tabuleiro para ela.\n4. Volte aqui e clique neste botão para que ela veja e aprenda a diferença.");
            dialog.run();
        }
    });
    aprender_menu->append(*corrigir_item);

    // --- Novo Menu "Rede" ---
    m_rede_menu_item = Gtk::make_managed<Gtk::MenuItem>("_Rede", true);
    auto rede_menu = Gtk::make_managed<Gtk::Menu>();
    m_rede_menu_item->set_submenu(*rede_menu);

    m_connect_item = Gtk::make_managed<Gtk::MenuItem>("_Conectar", true);
    m_connect_item->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_connect_activated));
    rede_menu->append(*m_connect_item);

    m_disconnect_item = Gtk::make_managed<Gtk::MenuItem>("_Desconectar", true);
    m_disconnect_item->set_sensitive(false);
    m_disconnect_item->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_disconnect_activated));
    rede_menu->append(*m_disconnect_item);

    // --- Menu "Tabuleiro" ---
    auto board_menu_item = Gtk::make_managed<Gtk::MenuItem>("_Tabuleiro", true);
    auto board_menu = Gtk::make_managed<Gtk::Menu>();
    board_menu_item->set_submenu(*board_menu);

    m_resize_item = Gtk::make_managed<Gtk::MenuItem>("_Redimensionar", true);
    m_resize_item->signal_activate().connect([this]() {
        // Dimensões base (tamanho pequeno)
        const int BASE_SQUARE_SIZE = 60;
        const int BASE_RULE_PADDING = 60;
        const int BASE_WINDOW_WIDTH = 690; // 480(tabuleiro) + 60(padding) + 150(painel de análise)
        const int BASE_WINDOW_HEIGHT = 570;

        int target_square_size;
        int target_rule_padding;
        int target_window_width;
        int target_window_height;

        if (m_is_large) {
            // Mudar para pequeno
            target_square_size = BASE_SQUARE_SIZE;
            target_rule_padding = BASE_RULE_PADDING;
            target_window_width = BASE_WINDOW_WIDTH;
            target_window_height = BASE_WINDOW_HEIGHT;
        } else {
            // Mudar para grande
            const int LARGE_WINDOW_HEIGHT = 800; // Altura alvo para o tamanho grande
            double scale_factor = static_cast<double>(LARGE_WINDOW_HEIGHT) / BASE_WINDOW_HEIGHT;

            target_square_size = static_cast<int>(std::round(BASE_SQUARE_SIZE * scale_factor));
            target_rule_padding = static_cast<int>(std::round(BASE_RULE_PADDING * scale_factor));
            target_window_width = static_cast<int>(std::round(BASE_WINDOW_WIDTH * scale_factor));
            target_window_height = LARGE_WINDOW_HEIGHT;
        }

        update_ui_dimensions(target_square_size, target_rule_padding, target_window_width, target_window_height);
        m_is_large = !m_is_large; // Inverte o estado
    });
    board_menu->append(*m_resize_item);

    m_flip_item = Gtk::make_managed<Gtk::MenuItem>("_Girar", true);
    m_flip_item->set_sensitive(true); // Habilitado para permitir o giro do tabuleiro
    m_flip_item->signal_activate().connect([this]() {
        m_is_flipped = !m_is_flipped; // Inverte o estado de rotação
        update_board_orientation();
    });
    board_menu->append(*m_flip_item);

    // NOVO: Submenu "Montar"
    m_setup_menu_item = Gtk::make_managed<Gtk::MenuItem>("_Montar", true);
    m_setup_menu_item->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_setup_board_activated));
    board_menu->append(*m_setup_menu_item);

    board_menu->append(*Gtk::make_managed<Gtk::SeparatorMenuItem>());

    // --- Voltar ---
    m_undo_item = Gtk::make_managed<Gtk::MenuItem>();
    auto undo_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 10);
    auto undo_label = Gtk::make_managed<Gtk::Label>("Voltar");
    auto undo_accel_label = Gtk::make_managed<Gtk::Label>("<<");
    undo_box->pack_start(*undo_label, Gtk::PACK_SHRINK);
    undo_box->pack_end(*undo_accel_label, Gtk::PACK_SHRINK);
    m_undo_item->add(*undo_box);
    m_undo_item->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_undo_activated));
    board_menu->append(*m_undo_item);

    // --- Avançar ---
    m_redo_item = Gtk::make_managed<Gtk::MenuItem>();
    auto redo_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 10);
    auto redo_label = Gtk::make_managed<Gtk::Label>("Avançar");
    auto redo_accel_label = Gtk::make_managed<Gtk::Label>(">>");
    redo_box->pack_start(*redo_label, Gtk::PACK_SHRINK);
    redo_box->pack_end(*redo_accel_label, Gtk::PACK_SHRINK);
    m_redo_item->add(*redo_box);
    m_redo_item->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_redo_activated));
    board_menu->append(*m_redo_item);

    // Adiciona os menus à barra na ordem definida
    m_menu_bar.append(*m_arquivo_menu_item);
    m_menu_bar.append(*board_menu_item);
    m_menu_bar.append(*m_mode_menu_item);
    m_menu_bar.append(*m_level_menu_item);
    m_menu_bar.append(*m_rede_menu_item);
    m_menu_bar.append(*m_aprender_menu_item);
    m_menu_bar.append(*help_menu_item);

    // --- Indicador Dinâmico de Rede ---
    m_network_status_label = Gtk::make_managed<Gtk::Label>("");
    m_network_status_item = Gtk::make_managed<Gtk::MenuItem>();
    m_network_status_item->add(*m_network_status_label);
    m_network_status_item->set_sensitive(false); // Apenas visual, não clicável
    m_network_status_item->set_hexpand(true); // Empurra o item para a extremidade direita da barra
    m_network_status_item->set_halign(Gtk::ALIGN_END);
    m_menu_bar.append(*m_network_status_item);

    // Aplica um estilo CSS para destacar a barra de menus da barra de título
    auto css_provider = Gtk::CssProvider::create();
    std::string css_data = 
        "menubar { background-color: #2c3e50; border-bottom: 2px solid #1a252f; } "
        "menubar > menuitem > label { color: #ffffff; } "
        "menubar > menuitem:hover { background-color: #34495e; } "
        "menubar > menuitem:hover > label { color: #ffffff; }";
    css_provider->load_from_data(css_data);
    m_menu_bar.get_style_context()->add_provider(css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

Gtk::Widget* MainWindow::create_board_area()
{
    // O Grid terá 10x10: 8x8 para o tabuleiro e 1 para cada régua.
    m_board_grid.set_row_spacing(2);
    m_board_grid.set_column_spacing(2);
    // Cria as réguas de coordenadas (letras e números).
    for (int i = 0; i < 8; ++i) {
        // Letras (a-h) nas réguas superior e inferior.
        char col_char = 'a' + i;
        auto top_label = Gtk::make_managed<Gtk::Label>(std::string(1, col_char));
        auto bottom_label = Gtk::make_managed<Gtk::Label>(std::string(1, col_char));
        m_board_grid.attach(*top_label, i + 1, 0, 1, 1);
        m_top_ruler_labels[i] = top_label;
        m_board_grid.attach(*bottom_label, i + 1, 9, 1, 1);
        m_bottom_ruler_labels[i] = bottom_label;

        // Números (8-1) nas réguas esquerda e direita.
        char row_char = '8' - i;
        auto left_label = Gtk::make_managed<Gtk::Label>(std::string(1, row_char));
        auto right_label = Gtk::make_managed<Gtk::Label>(std::string(1, row_char));
        m_board_grid.attach(*left_label, 0, i + 1, 1, 1);
        m_left_ruler_labels[i] = left_label;
        m_board_grid.attach(*right_label, 9, i + 1, 1, 1);
        m_right_ruler_labels[i] = right_label;
    }

    // Cria as casas do tabuleiro 8x8.
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            auto square = Gtk::make_managed<Gtk::DrawingArea>();
            // Define o tamanho inicial da casa usando a variável de membro
            square->set_size_request(m_current_square_size, m_current_square_size);
            square->add_events(Gdk::BUTTON_PRESS_MASK); // Habilita eventos de clique

            // Determina se a casa é clara ou escura, garantindo que a1 (canto inferior esquerdo) seja escura.
            // A lógica é invertida para que (row=7, col=0) resulte em uma casa escura.
            bool is_light = (row + col) % 2 == 0;

            // Conecta o sinal de clique
            // Conecta o sinal de desenho para colorir a casa.
            square->signal_draw().connect(
                sigc::bind(sigc::mem_fun(*this, &MainWindow::on_square_draw), row, col)
            );

            // Armazena o ponteiro bruto da DrawingArea no vetor para acesso posterior
            m_board_squares[row][col] = square;
            square->signal_button_press_event().connect(
                sigc::bind(sigc::mem_fun(*this, &MainWindow::on_board_click), row, col)
            );

            // Anexa a casa ao grid (col+1, row+1 por causa das réguas).
            m_board_grid.attach(*square, col + 1, row + 1, 1, 1);
        }
    }
    
    // Centraliza o grid na área disponível para que não se estique.
    auto alignment = Gtk::make_managed<Gtk::Alignment>(0.5, 0.5, 0, 0);
    alignment->add(m_board_grid);
    return alignment;
}

Gtk::Stack* MainWindow::create_right_panel() // Agora retorna Gtk::Stack*
{
    m_right_panel_stack.set_size_request(150, -1); // Fixa a largura do painel lateral em 150px

    // --- Painel 1: Análise e Histórico (Vista Normal) ---
    m_analysis_frame.set_border_width(5);
    m_analysis_frame.set_label_align(0.0, 0.5); // Alinha o título "Painel de análise" à esquerda

    m_analysis_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 5);
    m_analysis_frame.add(*m_analysis_box);

    m_pv_title_label = Gtk::make_managed<Gtk::Label>("Melhor linha");
    m_pv_title_label->set_halign(Gtk::ALIGN_START); 
    m_analysis_box->pack_start(*m_pv_title_label, Gtk::PACK_SHRINK);

    auto* pv_scrolled = Gtk::make_managed<Gtk::ScrolledWindow>();
    pv_scrolled->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    pv_scrolled->set_size_request(-1, 110); 
    m_pv_textview = Gtk::make_managed<Gtk::TextView>();
    m_pv_text_buffer = m_pv_textview->get_buffer();
    m_pv_textview->set_editable(false);
    m_pv_textview->set_cursor_visible(false);
    m_pv_textview->override_color(Gdk::RGBA("#6495ED")); 
    m_pv_textview->set_wrap_mode(Gtk::WRAP_WORD_CHAR); 
    
    Pango::FontDescription font;
    font.set_family("Monospace");
    font.set_size(9 * Pango::SCALE);
    m_pv_textview->override_font(font);

    pv_scrolled->add(*m_pv_textview);
    m_analysis_box->pack_start(*pv_scrolled, Gtk::PACK_SHRINK);

    auto* stats_grid = Gtk::make_managed<Gtk::Grid>();
    stats_grid->set_column_spacing(10);
    stats_grid->set_halign(Gtk::ALIGN_START);
    stats_grid->set_margin_start(5);

    auto set_red_bold = [](Gtk::Label* lbl, const std::string& txt) {
        lbl->set_markup("<span weight='bold' foreground='red'>" + txt + "</span>");
        lbl->set_xalign(0.0); 
    };

    auto title_pro = Gtk::make_managed<Gtk::Label>(); set_red_bold(title_pro, "Pro:");
    auto title_pos = Gtk::make_managed<Gtk::Label>(); set_red_bold(title_pos, "Pos:");
    auto title_sco = Gtk::make_managed<Gtk::Label>(); set_red_bold(title_sco, "Sco:");
    auto title_tem = Gtk::make_managed<Gtk::Label>(); set_red_bold(title_tem, "Tem:");

    m_depth_label_value = Gtk::make_managed<Gtk::Label>(); set_red_bold(m_depth_label_value, "-");
    m_nodes_label_value = Gtk::make_managed<Gtk::Label>(); set_red_bold(m_nodes_label_value, "-");
    m_score_label_value = Gtk::make_managed<Gtk::Label>(); set_red_bold(m_score_label_value, "-");
    m_time_label_value = Gtk::make_managed<Gtk::Label>();  set_red_bold(m_time_label_value, "-");

    stats_grid->attach(*title_pro, 0, 0);
    stats_grid->attach(*m_depth_label_value, 1, 0);
    stats_grid->attach(*title_pos, 0, 1);
    stats_grid->attach(*m_nodes_label_value, 1, 1);
    stats_grid->attach(*title_sco, 0, 2);
    stats_grid->attach(*m_score_label_value, 1, 2);
    stats_grid->attach(*title_tem, 0, 3);
    stats_grid->attach(*m_time_label_value, 1, 3);
    m_analysis_box->pack_start(*stats_grid, Gtk::PACK_SHRINK);

    m_v_paned_right->pack_start(m_analysis_frame, Gtk::PACK_SHRINK);

    // --- Painel 1: Histórico (Vista Normal) ---
    m_history_frame.set_border_width(5);
    m_history_frame.set_label_align(0.0, 0.5); // Alinha o título "Histórico de Lances" à esquerda
    m_history_scrolled_window.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    m_history_scrolled_window.add(m_history_text_view);
    m_history_frame.add(m_history_scrolled_window);
    m_v_paned_right->pack_start(m_history_frame, Gtk::PACK_EXPAND_WIDGET);

    m_history_text_view.set_editable(false);
    m_history_text_view.set_cursor_visible(false);
    m_history_text_view.set_wrap_mode(Gtk::WRAP_WORD_CHAR);

    m_right_panel_stack.add(*m_v_paned_right, "normal_panel");

    // --- Painel 2: Paleta de Montagem ---
    m_palette_vbox->set_border_width(10);
    m_palette_vbox->set_spacing(10);

    auto* tools_grid = Gtk::make_managed<Gtk::Grid>();
    tools_grid->set_halign(Gtk::ALIGN_CENTER);
    tools_grid->set_column_spacing(5);
    tools_grid->set_row_spacing(5);
    m_palette_vbox->pack_start(*tools_grid, Gtk::PACK_SHRINK);

    const int tool_size = 40;
    m_white_pawn_palette_item.set_size_request(tool_size, tool_size);
    m_black_pawn_palette_item.set_size_request(tool_size, tool_size);
    m_white_king_palette_item.set_size_request(tool_size, tool_size);
    m_black_king_palette_item.set_size_request(tool_size, tool_size);
    m_eraser_palette_item.set_size_request(tool_size, tool_size);

    m_white_pawn_palette_item.add_events(Gdk::BUTTON_PRESS_MASK);
    m_black_pawn_palette_item.add_events(Gdk::BUTTON_PRESS_MASK);
    m_white_king_palette_item.add_events(Gdk::BUTTON_PRESS_MASK);
    m_black_king_palette_item.add_events(Gdk::BUTTON_PRESS_MASK);
    m_eraser_palette_item.add_events(Gdk::BUTTON_PRESS_MASK);

    m_white_pawn_palette_item.signal_draw().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::draw_palette_item), PaletteTool::WHITE_PAWN));
    m_black_pawn_palette_item.signal_draw().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::draw_palette_item), PaletteTool::BLACK_PAWN));
    m_white_king_palette_item.signal_draw().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::draw_palette_item), PaletteTool::WHITE_KING));
    m_black_king_palette_item.signal_draw().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::draw_palette_item), PaletteTool::BLACK_KING));
    m_eraser_palette_item.signal_draw().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::draw_palette_item), PaletteTool::ERASER));

    m_white_pawn_palette_item.signal_button_press_event().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::on_palette_item_click), PaletteTool::WHITE_PAWN));
    m_black_pawn_palette_item.signal_button_press_event().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::on_palette_item_click), PaletteTool::BLACK_PAWN));
    m_white_king_palette_item.signal_button_press_event().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::on_palette_item_click), PaletteTool::WHITE_KING));
    m_black_king_palette_item.signal_button_press_event().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::on_palette_item_click), PaletteTool::BLACK_KING));
    m_eraser_palette_item.signal_button_press_event().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::on_palette_item_click), PaletteTool::ERASER));

    tools_grid->attach(m_white_pawn_palette_item, 0, 0);
    tools_grid->attach(m_black_pawn_palette_item, 1, 0);
    tools_grid->attach(m_white_king_palette_item, 0, 1);
    tools_grid->attach(m_black_king_palette_item, 1, 1);
    tools_grid->attach(m_eraser_palette_item, 0, 2, 2, 1);

    m_palette_vbox->pack_start(*Gtk::make_managed<Gtk::Separator>(Gtk::ORIENTATION_HORIZONTAL), Gtk::PACK_SHRINK);
    m_palette_vbox->pack_start(m_setup_clear_initial_button, Gtk::PACK_SHRINK);

    auto* turn_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL); // Empilha verticalmente para caber nos 120px
    turn_box->set_halign(Gtk::ALIGN_CENTER);
    turn_box->set_spacing(5);
    m_setup_black_turn_button.join_group(m_setup_white_turn_button);
    turn_box->pack_start(m_setup_white_turn_button);
    turn_box->pack_start(m_setup_black_turn_button);
    m_palette_vbox->pack_start(*turn_box, Gtk::PACK_SHRINK);

    m_palette_vbox->pack_start(m_setup_done_button, Gtk::PACK_SHRINK);

    m_setup_clear_initial_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_setup_clear_initial_clicked));
    m_setup_white_turn_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_setup_white_turn_clicked));
    m_setup_black_turn_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_setup_black_turn_clicked));
    m_setup_done_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_setup_done_clicked));

    m_right_panel_stack.add(*m_palette_vbox, "palette_panel");
    m_right_panel_stack.set_visible_child("normal_panel");

    return &m_right_panel_stack; // Retorna o ponteiro para o membro Gtk::Stack
}

// Helper para atualizar a orientação do tabuleiro
void MainWindow::update_board_orientation()
{
    // Atualiza o texto das réguas com base no estado de 'm_is_flipped'
    for (int i = 0; i < 8; ++i) {
        // Se girado, as colunas vão de 'h' a 'a'. Se não, de 'a' a 'h'.
        char col_char = m_is_flipped ? ('h' - i) : ('a' + i);
        // Se girado, as linhas vão de '1' a '8'. Se não, de '8' a '1'.
        char row_char = m_is_flipped ? ('1' + i) : ('8' - i);

        m_top_ruler_labels[i]->set_text(std::string(1, col_char));
        m_bottom_ruler_labels[i]->set_text(std::string(1, col_char));
        m_left_ruler_labels[i]->set_text(std::string(1, row_char));
        m_right_ruler_labels[i]->set_text(std::string(1, row_char));
    }

    // Força um redesenho de todo o tabuleiro.
    // A lógica de desenho de peças (a ser implementada) precisará verificar 'm_is_flipped'
    // para desenhar a peça correta na casa visual correta.
    m_board_grid.queue_draw();
}

// Implementação da função para montar o tabuleiro (agora ativa a paleta)
void MainWindow::on_setup_board_activated() {
    if (m_is_ai_turn_active) {
        m_discard_next_ai_move = true;
        DamasCore::interrupt_search();
    }
    m_in_setup_mode = true;
    set_menus_sensitive(false); // Desativa os menus para focar na montagem
    m_right_panel_stack.set_visible_child("palette_panel");
    m_selected_square = -1; // Limpa qualquer casa selecionada
    m_possible_moves_bitboard = 0; // Limpa movimentos possíveis
    m_possible_capture_moves.clear(); // Limpa capturas possíveis
    m_last_move_from_sq = -1; // Limpa o rastro
    m_last_move_to_sq = -1;
    m_board_grid.queue_draw(); // Redesenha o tabuleiro
    // No need to queue_draw on stack, it handles visibility changes
    update_undo_redo_sensitivity();
}

// Helper para atualizar as dimensões da UI
void MainWindow::update_ui_dimensions(int new_square_size, int new_rule_padding, int new_window_width, int new_window_height)
{
    m_current_square_size = new_square_size;
    m_current_rule_padding = new_rule_padding;

    // Atualiza o tamanho de cada casa do tabuleiro
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            if (m_board_squares[row][col]) {
                m_board_squares[row][col]->set_size_request(m_current_square_size, m_current_square_size);
            }
        }
    }

    // Redimensiona a janela. O signal_size_allocate do painel vertical cuidará de sua própria proporção.
    resize(new_window_width, new_window_height);
}

// Função chamada para desenhar cada casa do tabuleiro.
bool MainWindow::on_square_draw(const Cairo::RefPtr<Cairo::Context>& cr, int row, int col)
{
    // --- 1. Desenha o fundo da casa ---
    bool is_light = (row + col) % 2 == 0;
    Gdk::RGBA light_color("white"); // Branco
    Gdk::RGBA dark_color("green");  // Verde

    // Desenha o fundo da casa
    if (is_light) {
        Gdk::Cairo::set_source_rgba(cr, light_color);
    } else {
        Gdk::Cairo::set_source_rgba(cr, dark_color);
    }
    cr->rectangle(0, 0, m_current_square_size, m_current_square_size);
    cr->fill();

    // --- 2. Desenha destaques de seleção/movimento ---
    // Traduz as coordenadas físicas (row, col) para as coordenadas lógicas do tabuleiro
    const int logical_row = m_is_flipped ? (7 - row) : row;
    const int logical_col = m_is_flipped ? (7 - col) : col;
    const int logical_sq = logical_row * 8 + logical_col;
    const DamasCore::BoardBitboard logical_sq_mask = (1ULL << logical_sq);
    const Gdk::RGBA highlight_color("yellow");

    auto draw_highlight_border = [&](const Gdk::RGBA& color) {
        Gdk::Cairo::set_source_rgba(cr, color);
        cr->set_line_width(4.0);
        cr->rectangle(2.0, 2.0, m_current_square_size - 4.0, m_current_square_size - 4.0);
        cr->stroke();
    };

    // Rastro do último movimento
    if (logical_sq == m_last_move_from_sq || logical_sq == m_last_move_to_sq) {
        draw_highlight_border(highlight_color);
    }

    // Destaque da peça selecionada e seus destinos (sobrescreve o rastro se houver sobreposição)
    if (m_selected_square != -1) {
        if (logical_sq == m_selected_square) {
            // Borda de destaque para a peça selecionada (origem)
            draw_highlight_border(highlight_color);
        } else if ((m_possible_moves_bitboard & logical_sq_mask) != 0) {
            // Borda de destaque para os destinos possíveis
            draw_highlight_border(highlight_color);
        }
    }

    // --- 3. Verifica se há uma peça e a desenha (sempre por último) ---
    if ((m_game_state.obter_brancas_peoes() & logical_sq_mask) != 0) {
        draw_piece(cr, DamasCore::GamePlayer::BRANCAS, false, m_current_square_size);
    } else if ((m_game_state.obter_pretas_peoes() & logical_sq_mask) != 0) {
        draw_piece(cr, DamasCore::GamePlayer::PRETOS, false, m_current_square_size);
    } else if ((m_game_state.obter_brancas_damas() & logical_sq_mask) != 0) {
        draw_piece(cr, DamasCore::GamePlayer::BRANCAS, true, m_current_square_size);
    } else if ((m_game_state.obter_pretas_damas() & logical_sq_mask) != 0) {
        draw_piece(cr, DamasCore::GamePlayer::PRETOS, true, m_current_square_size);
    }

    return true;
}

bool MainWindow::on_board_click(GdkEventButton* event, int row, int col)
{
    // Ignora cliques no tabuleiro se a IA estiver pensando, exceto no modo Análise
    if (m_is_ai_turn_active && m_game_mode != GameMode::ANALYSIS) {
        return true;
    }

    // Traduz as coordenadas físicas (row, col) para as coordenadas lógicas do tabuleiro
    const int logical_row = m_is_flipped ? (7 - row) : row;
    const int logical_col = m_is_flipped ? (7 - col) : col;
    const int logical_sq = logical_row * 8 + logical_col;
    const DamasCore::BoardBitboard logical_sq_mask = (1ULL << logical_sq);

    // Lógica para o modo de montagem (lida com cliques esquerdo e direito)
    if (m_in_setup_mode) {
        if (event->type == GDK_BUTTON_PRESS) {
            if (event->button == 1) { // Botão esquerdo
                // Apenas permite modificação em casas escuras
                if ((logical_row + logical_col) % 2 != 0) {
                    DamasCore::BoardBitboard clicked_mask = (1ULL << logical_sq);

                    // Primeiro, remove qualquer peça existente na casa clicada
                    m_game_state.remover_brancas_peoes(clicked_mask);
                    m_game_state.remover_pretas_peoes(clicked_mask);
                    m_game_state.remover_brancas_damas(clicked_mask);
                    m_game_state.remover_pretas_damas(clicked_mask);

                        // Adiciona a nova peça de acordo com a ferramenta selecionada,
                        // impedindo a colocação de peões nas casas de promoção.
                    switch (m_selected_palette_tool) {
                            case PaletteTool::WHITE_PAWN: 
                                if ((clicked_mask & DamasCore::RANK_ULTIMA_BRANCA) == 0) {
                                    m_game_state.adicionar_brancas_peoes(clicked_mask); 
                                }
                                break;
                            case PaletteTool::BLACK_PAWN: 
                                if ((clicked_mask & DamasCore::RANK_PRIMEIRA_PRETA) == 0) {
                                    m_game_state.adicionar_pretas_peoes(clicked_mask); 
                                }
                                break;
                        case PaletteTool::WHITE_KING: m_game_state.adicionar_brancas_damas(clicked_mask); break;
                        case PaletteTool::BLACK_KING: m_game_state.adicionar_pretas_damas(clicked_mask); break;
                        case PaletteTool::ERASER: // A peça já foi removida, nada a fazer
                        case PaletteTool::NONE:   // Nenhuma ferramenta selecionada, nada a fazer
                            break;
                    }
                    m_game_state.recalcular_hash_completo();
                    m_board_grid.queue_draw();
                } else {
                }
                return true; // Evento tratado
            }
            else if (event->button == 3) { // Botão direito
                Gtk::Menu context_menu;

                auto* white_pawn_item = Gtk::make_managed<Gtk::MenuItem>("Peão Branco");
                auto* black_pawn_item = Gtk::make_managed<Gtk::MenuItem>("Peão Preto");
                auto* white_king_item = Gtk::make_managed<Gtk::MenuItem>("Dama Branca");
                auto* black_king_item = Gtk::make_managed<Gtk::MenuItem>("Dama Preta");
                auto* eraser_item = Gtk::make_managed<Gtk::MenuItem>("Borracha (Apagar)");
                auto* none_item = Gtk::make_managed<Gtk::MenuItem>("Nenhum (Desselecionar)");

                white_pawn_item->signal_activate().connect([this]() { this->set_palette_tool(PaletteTool::WHITE_PAWN); });
                black_pawn_item->signal_activate().connect([this]() { this->set_palette_tool(PaletteTool::BLACK_PAWN); });
                white_king_item->signal_activate().connect([this]() { this->set_palette_tool(PaletteTool::WHITE_KING); });
                black_king_item->signal_activate().connect([this]() { this->set_palette_tool(PaletteTool::BLACK_KING); });
                eraser_item->signal_activate().connect([this]() { this->set_palette_tool(PaletteTool::ERASER); });
                none_item->signal_activate().connect([this]() { this->set_palette_tool(PaletteTool::NONE); });

                context_menu.append(*white_pawn_item);
                context_menu.append(*black_pawn_item);
                context_menu.append(*white_king_item);
                context_menu.append(*black_king_item);
                context_menu.append(*Gtk::make_managed<Gtk::SeparatorMenuItem>());
                context_menu.append(*eraser_item);
                context_menu.append(*none_item);

                context_menu.show_all();
                context_menu.popup_at_pointer((const GdkEvent*)event);

                return true; // Evento tratado
            }
        }
        return true; // Em modo de montagem, ignora outros tipos de evento
    }

    // Lógica para o modo de jogo normal (só lida com clique esquerdo)
    if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
        
        // Impede interações manuais no tabuleiro se estiver em uma partida de rede e for a vez do oponente
        if (m_dxp_client.is_conectado() && m_game_state.obter_turno_atual() != m_minha_cor_dxp) {
            return true;
        }
        
        // --- Cenário 1: Uma peça já está selecionada ---
        if (m_selected_square != -1) {
            // ... (código de jogo normal continua aqui)
            
            // Verifica se o clique é em uma casa de movimento possível
            if (!m_possible_capture_moves.empty()) { // Se há capturas disponíveis
                // Procura TODOS os movimentos de captura que correspondem ao clique
                DamasCore::GameMove temp_matches[16];
                int match_count = 0;
                const auto* cap_ptr = m_possible_capture_moves.data();
                size_t cap_size = m_possible_capture_moves.size();
                for (size_t i = 0; i < cap_size; ++i) {
                    if (cap_ptr[i].casa_origem == m_selected_square && cap_ptr[i].casa_destino == logical_sq) {
                        if (match_count < 16) temp_matches[match_count++] = cap_ptr[i];
                    }
                }

                if (match_count == 1) { // Lance único e não ambíguo
                    DamasCore::GameMove selected_capture_move = temp_matches[0];
                    m_game_state.aplicar_movimento_completo_captura(selected_capture_move);
                    
                    // Se o humano seguiu a linha prevista pela IA, remove o lance da fila, senão limpa a memória para re-análise
                    if (!m_saved_win_pv.empty()) {
                        if (m_saved_win_pv.front() == selected_capture_move) m_saved_win_pv.erase(m_saved_win_pv.begin());
                        else m_saved_win_pv.clear();
                    }

                    // Após uma captura, o turno sempre termina.
                    m_game_state.alternar_turno();
                    std::string move_notation = get_algebraic_notation(selected_capture_move.casa_origem, logical_sq, selected_capture_move.mascaras_pecas_capturadas);
                    m_last_move_from_sq = selected_capture_move.casa_origem; // Registra o rastro
                    m_last_move_to_sq = logical_sq;                     // Registra o rastro
                    record_new_state(move_notation); // Grava o novo estado com a notação
                    play_move_sound(); // Toca o som do movimento

                    if (m_is_ai_turn_active && m_game_mode == GameMode::ANALYSIS) {
                        m_discard_next_ai_move = true;
                        DamasCore::interrupt_search();
                    }

                    // Limpa o estado de seleção após o lance
                    m_selected_square = -1;
                    m_possible_moves_bitboard = 0;
                    m_possible_capture_moves.clear();
                } else if (match_count > 1) { // Lance ambíguo
                    // Armazena os lances, limpa o estado de seleção e mostra o diálogo de escolha.
                    m_selected_square = -1;
                    m_possible_moves_bitboard = 0;
                    m_possible_capture_moves.clear();
                    m_board_grid.queue_draw(); // Redesenha para remover os destaques

                    show_ambiguous_capture_dialog(temp_matches, match_count);
                } else {
                    // Clicou em uma casa que não é destino de nenhuma captura válida
                    m_selected_square = -1;
                    m_possible_moves_bitboard = 0;
                    m_possible_capture_moves.clear();
                    m_mandatory_capture_origins = 0; // Limpa também os destaques de captura obrigatória
                }
            } else if ((m_possible_moves_bitboard & logical_sq_mask) != 0) { // Se há movimentos simples disponíveis
                m_game_state.aplicar_mov_simples(m_selected_square, logical_sq);
                DamasCore::GameMove simple_mv{m_selected_square, logical_sq, 0};
                
                if (!m_saved_win_pv.empty()) {
                    if (m_saved_win_pv.front() == simple_mv) m_saved_win_pv.erase(m_saved_win_pv.begin());
                    else m_saved_win_pv.clear();
                }

                // Após um movimento simples, o turno sempre termina.
                m_game_state.alternar_turno();
                std::string move_notation = get_algebraic_notation(m_selected_square, logical_sq, 0);
                m_last_move_from_sq = m_selected_square; // Registra o rastro
                m_last_move_to_sq = logical_sq;         // Registra o rastro
                record_new_state(move_notation); // Grava o novo estado com a notação
                play_move_sound(); // Toca o som do movimento

                if (m_is_ai_turn_active && m_game_mode == GameMode::ANALYSIS) {
                    m_discard_next_ai_move = true;
                    DamasCore::interrupt_search();
                }

                m_selected_square = -1;
                m_possible_moves_bitboard = 0;
                m_mandatory_capture_origins = 0; // Limpa também os destaques de captura obrigatória
            } else { // Clicou em uma casa inválida (nem captura, nem movimento simples)
                m_selected_square = -1;
                m_possible_moves_bitboard = 0;
                m_possible_capture_moves.clear();
            }
            m_board_grid.queue_draw(); // Redesenha o tabuleiro para mostrar o lance do jogador
            if (!check_game_over_state() || m_game_mode == GameMode::ANALYSIS) {
                check_for_ai_move(); // Se o jogo não acabou, verifica se é a vez da IA jogar
            }
        } // End of Scenario 1: Piece already selected
        // --- Cenário 2: Nenhuma peça selecionada ---
        else {
            const DamasCore::GamePlayer current_turn = m_game_state.obter_turno_atual();
            DamasCore::BoardBitboard player_pieces = 0; // Todas as peças do jogador atual (peões e damas)
            std::string player_color_str;

            if (current_turn == DamasCore::GamePlayer::BRANCAS) {
                player_pieces = m_game_state.obter_todas_pecas_brancas();
                player_color_str = "branca";
            } else { // GamePlayer::PRETOS
                player_pieces = m_game_state.obter_todas_pecas_pretas();
                player_color_str = "preta";
            }

            // Verifica se a casa clicada contém uma peça (peão ou dama) do jogador atual
            if ((player_pieces & logical_sq_mask) != 0) {
                // Regra de Captura Obrigatória (Lei da Maioria):
                m_possible_capture_moves = DamasCore::gerar_todas_capturas_maximais(m_game_state);

                m_mandatory_capture_origins = 0; // Limpa os destaques anteriores
                const auto* cap_ptr1 = m_possible_capture_moves.data();
                size_t cap_size1 = m_possible_capture_moves.size();
                for (size_t i = 0; i < cap_size1; ++i) {
                    m_mandatory_capture_origins |= (1ULL << cap_ptr1[i].casa_origem);
                }

                if (!m_possible_capture_moves.empty()) {
                    bool can_start_capture = false;
                    const auto* cap_ptr2 = m_possible_capture_moves.data();
                    size_t cap_size2 = m_possible_capture_moves.size();
                    for (size_t i = 0; i < cap_size2; ++i) {
                        if (cap_ptr2[i].casa_origem == logical_sq) {
                            can_start_capture = true;
                            break;
                        }
                    }

                    if (can_start_capture) {
                        m_selected_square = logical_sq;
                        m_possible_moves_bitboard = 0;
                        const auto* cap_ptr3 = m_possible_capture_moves.data();
                        size_t cap_size3 = m_possible_capture_moves.size();
                        for (size_t i = 0; i < cap_size3; ++i) {
                            if (cap_ptr3[i].casa_origem == m_selected_square) {
                                m_possible_moves_bitboard |= (1ULL << cap_ptr3[i].casa_destino);
                            }
                        }
                        m_board_grid.queue_draw();
                    } else {
                        // Limpa qualquer seleção ou destaque anterior
                        m_selected_square = -1;
                        m_possible_moves_bitboard = 0;
                        m_possible_capture_moves.clear();
                        // m_mandatory_capture_origins já foi populado, então o redraw vai mostrar as peças corretas
                        m_board_grid.queue_draw();
                    }
                } else {
                    bool is_clicked_pawn = ((m_game_state.obter_brancas_peoes() | m_game_state.obter_pretas_peoes()) & logical_sq_mask) != 0;
                    m_selected_square = logical_sq;
                    if (is_clicked_pawn) {
                        m_possible_moves_bitboard = DamasCore::gerar_movimentos_simples_peao(m_game_state, logical_sq);
                    } else { // É uma dama
                        m_possible_moves_bitboard = DamasCore::gerar_movimentos_simples_dama(m_game_state, logical_sq);
                    }
                    m_board_grid.queue_draw(); // Redesenha para mostrar destaques
                }
            }
        }
    }

    return true; // Indica que o evento foi tratado
}

std::string MainWindow::get_captured_squares_string(DamasCore::BoardBitboard captured_mask) {
    std::stringstream ss;
    bool first = true;
    DamasCore::BoardBitboard temp_mask = captured_mask;
    while (temp_mask != 0) {
        int sq = __builtin_ctzll(temp_mask);
        if (!first) {
            ss << ", ";
        }
        ss << (char)('a' + (sq % 8));
        ss << (char)('8' - (sq / 8));
        first = false;
        temp_mask &= temp_mask - 1;
    }
    return ss.str();
}

void MainWindow::show_ambiguous_capture_dialog(const DamasCore::GameMove* moves_ptr, size_t count)
{
    Gtk::Dialog dialog("Escolha a Captura", *this, true);
    dialog.set_default_size(350, 100);

    Gtk::Box* content_area = dialog.get_content_area();
    content_area->set_spacing(10);
    content_area->set_border_width(10);

    Gtk::Label info_label("Múltiplas capturas possíveis para a mesma casa de destino.\nEscolha qual sequência de peças capturar:");
    content_area->pack_start(info_label, Gtk::PACK_SHRINK);

    Gtk::Box vbox(Gtk::ORIENTATION_VERTICAL, 5);
    content_area->pack_start(vbox, Gtk::PACK_EXPAND_WIDGET);

    for (size_t i = 0; i < count; ++i) {
        std::string captured_pieces_str = get_captured_squares_string(moves_ptr[i].mascaras_pecas_capturadas);
        std::string button_text = "Capturar via: " + captured_pieces_str;
        auto* button = Gtk::make_managed<Gtk::Button>(button_text);
        button->signal_clicked().connect([&dialog, i]() {
            dialog.response(i + 1); // Usa i+1 como ID de resposta
        });
        vbox.pack_start(*button, Gtk::PACK_SHRINK);
    }

    dialog.add_button("Cancelar", Gtk::RESPONSE_CANCEL);
    dialog.show_all();

    int result = dialog.run();

    if (result > 0 && result <= static_cast<int>(count)) {
        DamasCore::GameMove selected_move = moves_ptr[result - 1];
        
        // Executa o lance de captura escolhido
        m_game_state.aplicar_movimento_completo_captura(selected_move);
        if (!m_saved_win_pv.empty()) {
            if (m_saved_win_pv.front() == selected_move) m_saved_win_pv.erase(m_saved_win_pv.begin());
            else m_saved_win_pv.clear();
        }

        m_game_state.alternar_turno();
        std::string move_notation = get_algebraic_notation(selected_move.casa_origem, selected_move.casa_destino, selected_move.mascaras_pecas_capturadas);
        m_last_move_from_sq = selected_move.casa_origem;
        m_last_move_to_sq = selected_move.casa_destino;
        record_new_state(move_notation);
        play_move_sound(); // Toca o som do movimento

        if (m_is_ai_turn_active && m_game_mode == GameMode::ANALYSIS) {
            m_discard_next_ai_move = true;
            DamasCore::interrupt_search();
        }

        if (!check_game_over_state() || m_game_mode == GameMode::ANALYSIS) {
            check_for_ai_move();
        }
    }
}

void MainWindow::draw_piece(const Cairo::RefPtr<Cairo::Context>& cr, DamasCore::GamePlayer player, bool is_king, double piece_size)
{
    const double center_x = piece_size / 2.0;
    const double center_y = piece_size / 2.0;
    const double radius = piece_size * 0.4; // A peça ocupa 80% do diâmetro (40% do tamanho da casa)

    // --- Desenha o corpo da peça com um gradiente para efeito 3D ---
    Cairo::RefPtr<Cairo::RadialGradient> gradient;

    if (player == DamasCore::GamePlayer::BRANCAS) {
        gradient = Cairo::RadialGradient::create(
            center_x - radius * 0.2, center_y - radius * 0.2, 0, // Centro do brilho
            center_x, center_y, radius);
        gradient->add_color_stop_rgba(0.0, 1.0, 1.0, 1.0, 1.0); // Branco brilhante no centro
        gradient->add_color_stop_rgba(1.0, 0.8, 0.8, 0.8, 1.0); // Cinza claro na borda
    } else { // BLACK
        gradient = Cairo::RadialGradient::create(
            center_x - radius * 0.2, center_y - radius * 0.2, 0,
            center_x, center_y, radius);
        gradient->add_color_stop_rgba(0.0, 0.5, 0.5, 0.5, 1.0); // Cinza médio no centro
        gradient->add_color_stop_rgba(1.0, 0.1, 0.1, 0.1, 1.0); // Quase preto na borda
    }

    cr->set_source(gradient);
    cr->arc(center_x, center_y, radius, 0, 2 * M_PI);
    cr->fill_preserve(); // Preenche e mantém o caminho para a borda

    // --- Desenha uma borda sutil para definição ---
    cr->set_line_width(1.5);
    cr->set_source_rgba(0, 0, 0, 0.6); // Borda escura semi-transparente
    cr->stroke();

    // --- Desenha o indicador de Dama (um círculo amarelo) ---
    if (is_king) {
        const double inner_radius = radius * 0.5; // Define o tamanho do círculo interno (50% do raio da peça)
        cr->arc(center_x, center_y, inner_radius, 0, 2 * M_PI);
        Gdk::Cairo::set_source_rgba(cr, Gdk::RGBA("yellow")); // Altera a cor para amarelo
        cr->fill();
    }
}

bool MainWindow::draw_palette_item(const Cairo::RefPtr<Cairo::Context>& cr, PaletteTool tool)
{
    const int size = 40;
    // Fundo
    cr->rectangle(0, 0, size, size);
    if (m_selected_palette_tool == tool) {
        Gdk::Cairo::set_source_rgba(cr, Gdk::RGBA("lightblue")); // Destaque para ferramenta selecionada
    } else {
        Gdk::Cairo::set_source_rgba(cr, Gdk::RGBA("lightgrey")); // Fundo padrão
    }
    cr->fill();

    switch (tool) {
        case PaletteTool::WHITE_PAWN:
            draw_piece(cr, DamasCore::GamePlayer::BRANCAS, false, size);
            break;
        case PaletteTool::BLACK_PAWN:
            draw_piece(cr, DamasCore::GamePlayer::PRETOS, false, size);
            break;
        case PaletteTool::WHITE_KING:
            draw_piece(cr, DamasCore::GamePlayer::BRANCAS, true, size);
            break;
        case PaletteTool::BLACK_KING:
            draw_piece(cr, DamasCore::GamePlayer::PRETOS, true, size);
            break;
        case PaletteTool::ERASER:
            {
                // Desenha uma borracha simples
                cr->save();
                cr->translate(size / 2.0, size / 2.0);
                cr->rotate(M_PI / 4.0); // Rotaciona 45 graus

                const double w = size * 0.3;
                const double h = size * 0.15;
                cr->rectangle(-w, -h, w * 2, h * 2);
                Gdk::Cairo::set_source_rgba(cr, Gdk::RGBA("pink"));
                cr->fill_preserve();

                cr->set_line_width(2.0);
                Gdk::Cairo::set_source_rgba(cr, Gdk::RGBA("darkred"));
                cr->stroke();
                cr->restore();
            }
            break;
        case PaletteTool::NONE:
            break;
    }

    return true;
}

bool MainWindow::on_palette_item_click(GdkEventButton* event, PaletteTool tool)
{
    // A seleção da ferramenta na paleta é com o botão esquerdo.
    if (event->type == GDK_BUTTON_PRESS && event->button == 1) { // Botão esquerdo
        // Alterna a seleção: se clicar na mesma ferramenta, deseleciona. Senão, seleciona a nova.
        PaletteTool new_tool = (m_selected_palette_tool == tool) ? PaletteTool::NONE : tool;
        set_palette_tool(new_tool);
        return true; // Evento tratado
    }
    return false; // Evento não tratado (e.g., clique direito na paleta)
}

void MainWindow::on_setup_clear_initial_clicked() {
    // Se o tabuleiro estiver atualmente limpo, restaura a posição inicial.
    if (m_is_board_cleared) {
        DamasCore::BoardState initial_state; initial_state.configurar_posicao_inicial();
        reset_game_history(initial_state, "Posição Inicial (12x12)");
        m_is_board_cleared = false;
    } 
    // Se o tabuleiro não estiver limpo (está na posição inicial ou alguma outra), limpa-o.
    else {
        DamasCore::BoardState cleared_state; cleared_state.limpar_tabuleiro();
        reset_game_history(cleared_state, "Tabuleiro Limpo");
        m_is_board_cleared = true;
    }
}

void MainWindow::on_setup_white_turn_clicked() {
    m_setup_turn = DamasCore::GamePlayer::BRANCAS;
}

void MainWindow::on_setup_black_turn_clicked() {
    m_setup_turn = DamasCore::GamePlayer::PRETOS;
}

void MainWindow::on_setup_done_clicked() {
    m_in_setup_mode = false;
    set_menus_sensitive(true); // Reativa os menus
    m_game_state.definir_turno_atual(m_setup_turn); 
    m_game_state.recalcular_hash_completo();
    m_selected_palette_tool = PaletteTool::NONE;
    m_palette_vbox->queue_draw(); // Redesenha a paleta para remover destaques

    m_right_panel_stack.set_visible_child("normal_panel");
    reset_game_history(m_game_state, "Configuração Manual"); // Inicia um novo histórico com a configuração manual
}

void MainWindow::reset_game_history(const DamasCore::BoardState& initial_state, const std::string& initial_notation) {
    // Disconecta qualquer timer pendente de encadeamento de IA para evitar chamadas inesperadas
    if (m_ai_chain_timer_conn) {
        m_ai_chain_timer_conn.disconnect();
    }

    m_game_state = initial_state;
    m_history_count = 0;
    m_history_stack[m_history_count++] = {m_game_state, initial_notation, -1, -1};
    m_history_index = 0;
    m_selected_square = -1;
    m_possible_moves_bitboard = 0;
    m_possible_capture_moves.clear();
    m_mandatory_capture_origins = 0;
    m_last_move_from_sq = -1;
    m_last_move_to_sq = -1;
    m_saved_win_pv.clear();
    m_board_grid.queue_draw();
    update_history_text_view();
    update_undo_redo_sensitivity();
}

void MainWindow::set_game_mode(GameMode mode) {
    if (m_ai_chain_timer_conn) m_ai_chain_timer_conn.disconnect();

    if (m_is_ai_turn_active) {
        m_discard_next_ai_move = true;
        DamasCore::interrupt_search();
    }
    m_game_mode = mode;
    check_for_ai_move();
}

void MainWindow::set_menus_sensitive(bool sensitive) {
    if (m_arquivo_menu_item) m_arquivo_menu_item->set_sensitive(sensitive);
    if (m_mode_menu_item) m_mode_menu_item->set_sensitive(sensitive);
    if (m_level_menu_item) m_level_menu_item->set_sensitive(sensitive);
    if (m_rede_menu_item) m_rede_menu_item->set_sensitive(sensitive);
    if (m_aprender_menu_item) m_aprender_menu_item->set_sensitive(sensitive);
    if (m_resize_item) m_resize_item->set_sensitive(sensitive);
    if (m_flip_item) m_flip_item->set_sensitive(sensitive);
    // O próprio botão "Montar" também é desativado para evitar reentrar no modo
    if (m_setup_menu_item) m_setup_menu_item->set_sensitive(sensitive);
}

void MainWindow::check_for_ai_move() {
    // Bloqueia qualquer tentativa de iniciar a IA enquanto o modo de montagem estiver ativo.
    // A IA só deve ser acionada após o usuário clicar em "Pronto".
    if (m_in_setup_mode) {
        return;
    }

    bool is_ai_turn = false;

    if (m_game_mode == GameMode::ANALYSIS) {
        is_ai_turn = true;
    } else if (m_game_mode == GameMode::HUM_VS_AI && m_game_state.obter_turno_atual() == DamasCore::GamePlayer::PRETOS) {
        is_ai_turn = true;
    } else if (m_game_mode == GameMode::AI_VS_HUM && m_game_state.obter_turno_atual() == DamasCore::GamePlayer::BRANCAS) {
        is_ai_turn = true;
    } else if (m_game_mode == GameMode::AI_VS_AI) {
        is_ai_turn = true; 
    }

    if (m_dxp_client.is_conectado() && m_game_state.obter_turno_atual() != m_minha_cor_dxp) {
        is_ai_turn = false;
    }

    if (is_ai_turn && !m_is_ai_turn_active) { 
        if (!m_saved_win_pv.empty() && m_game_mode != GameMode::ANALYSIS) {
            DamasCore::GameMove next_move = m_saved_win_pv.front();
            bool is_valid = false;
            
            DamasCore::MoveList caps = DamasCore::gerar_todas_capturas_maximais(m_game_state);
            if (!caps.empty()) {
                for (size_t i = 0; i < caps.size(); ++i) if (caps[i] == next_move) is_valid = true;
            } else {
                DamasCore::BoardBitboard bb = (m_game_state.obter_turno_atual() == DamasCore::GamePlayer::BRANCAS) ? m_game_state.obter_todas_pecas_brancas() : m_game_state.obter_todas_pecas_pretas();
                while (bb != 0) {
                    int sq = __builtin_ctzll(bb);
                    bool is_pawn = ((m_game_state.obter_brancas_peoes() | m_game_state.obter_pretas_peoes()) & (1ULL << sq)) != 0;
                    DamasCore::BoardBitboard dests = is_pawn ? DamasCore::gerar_movimentos_simples_peao(m_game_state, sq) : DamasCore::gerar_movimentos_simples_dama(m_game_state, sq);
                    if (next_move.casa_origem == sq && (dests & (1ULL << next_move.casa_destino))) {
                        is_valid = true; break;
                    }
                    bb &= bb - 1;
                }
            }

            if (is_valid) {
                m_is_ai_turn_active = true;
                m_ai_result_so.move = next_move;
                m_ai_result_so.score = 20000;
                m_saved_win_pv.erase(m_saved_win_pv.begin());
                Glib::signal_timeout().connect([this]() { m_ai_move_dispatcher.emit(); return false; }, 500);
                return;
            } else {
                m_saved_win_pv.clear(); 
            }
        }

        m_is_ai_turn_active = true;
        
        if (m_ai_thread.joinable()) {
            m_ai_thread.join();
        }

        m_ai_thread = std::thread([this]() {
            DamasCore::BoardState current_state_copy = m_game_state; 
            DamasCore::Search_Input si;
            si.init();
            si.time = (m_game_mode == GameMode::ANALYSIS) ? 86400.0 : m_ai_time_limit; 
            
            DamasCore::search(m_ai_result_so, current_state_copy, si);
            
            m_ai_move_dispatcher.emit();
        });
    }
}

void MainWindow::on_ai_move_ready()
{
    if (m_ai_thread.joinable()) {
        m_ai_thread.join();
    }

    if (m_discard_next_ai_move) {
        m_discard_next_ai_move = false;
        m_is_ai_turn_active = false;
        if (m_game_mode == GameMode::ANALYSIS && !m_in_setup_mode) {
            check_for_ai_move(); 
        }
        return;
    }

    on_update_analysis_timer();

    bool is_forced_win_move = (m_ai_result_so.score > 19000);
    
    if (is_forced_win_move && !m_ai_result_so.pv.empty() && m_saved_win_pv.empty()) {
        m_saved_win_pv = m_ai_result_so.pv;
        if (m_saved_win_pv.front() == m_ai_result_so.move) {
            m_saved_win_pv.erase(m_saved_win_pv.begin());
        }
    }

    if (m_game_mode == GameMode::ANALYSIS) {
        if (is_forced_win_move) {
            if (m_hum_vs_hum_item) m_hum_vs_hum_item->set_active(true);
            m_game_mode = GameMode::HUM_VS_HUM; 
        } else {
            m_is_ai_turn_active = false;
            return; 
        }
    }

    m_is_ai_turn_active = false; 

    DamasCore::GameMove ai_move = m_ai_result_so.move;

    if (ai_move.casa_origem == -1) {
        if (m_dxp_client.is_conectado()) {
            m_dxp_client.enviar_game_end('1', '0'); // Envia '1' (You win) indicando derrota/falta de lances
        }
        return;
    }

    m_selected_square = -1;
    m_possible_moves_bitboard = 0;
    m_possible_capture_moves.clear();
    m_mandatory_capture_origins = 0;

    if (ai_move.mascaras_pecas_capturadas != 0) {
        m_game_state.aplicar_movimento_completo_captura(ai_move);
    } else {
        m_game_state.aplicar_mov_simples(ai_move.casa_origem, ai_move.casa_destino);
    }

    m_game_state.alternar_turno();
    std::string move_notation = get_algebraic_notation(ai_move.casa_origem, ai_move.casa_destino, ai_move.mascaras_pecas_capturadas);
    m_last_move_from_sq = ai_move.casa_origem;
    m_last_move_to_sq = ai_move.casa_destino;
    record_new_state(move_notation); 
    play_move_sound(); // Toca o som do movimento da IA

    m_board_grid.queue_draw(); 

    if (m_dxp_client.is_conectado()) {
        int time_spent = static_cast<int>(m_ai_result_so.time_spent);

        auto para_dxp = [](int sq) {
            int r = sq / 8;
            int f = sq % 8;
            return (r * 4) + (f / 2) + 1;
        };
        
        int cap_sqs[32];
        int cap_idx = 0;
        DamasCore::BoardBitboard cap_mask = ai_move.mascaras_pecas_capturadas;
        while(cap_mask != 0) {
            int sq = __builtin_ctzll(cap_mask);
            if (cap_idx < 32) cap_sqs[cap_idx++] = para_dxp(sq);
            cap_mask &= cap_mask - 1;
        }
        
        m_dxp_client.enviar_movimento(time_spent, para_dxp(ai_move.casa_origem), para_dxp(ai_move.casa_destino), cap_sqs, cap_idx);
    }

    if (check_game_over_state()) {
        return;
    }

    if (m_game_mode == GameMode::AI_VS_AI) {
        if (m_dxp_client.is_conectado() && m_game_state.obter_turno_atual() != m_minha_cor_dxp) {
            return;
        }

        if (m_ai_chain_timer_conn) m_ai_chain_timer_conn.disconnect();
        m_ai_chain_timer_conn = Glib::signal_timeout().connect([this]() {
            check_for_ai_move();
            return false; 
        }, 100);
    }
}

bool MainWindow::on_update_analysis_timer() {
    if (!m_is_ai_turn_active) {
        return true; 
    }

    DamasCore::Search_Output so = DamasCore::get_current_so();
    if (!m_depth_label_value || so.depth == 0) return true;

    auto set_red_bold = [](Gtk::Label* lbl, const std::string& txt) {
        lbl->set_markup("<span weight='bold' foreground='red'>" + txt + "</span>");
    };

    set_red_bold(m_depth_label_value, std::to_string(so.depth));
    set_red_bold(m_nodes_label_value, format_nodes(so.node));
    
    std::string score_str;
    if (so.score > 19000) {
        int dtm = 20000 - so.score;
        score_str = "W+" + std::to_string(dtm);
    } else if (so.score < -19000) {
        int dtm = so.score + 20000;
        score_str = "L+" + std::to_string(dtm);
    } else if (so.depth == 100 && so.score == 0) {
        score_str = "D+0"; 
    } else {
        std::stringstream score_ss;
        score_ss << std::fixed << std::setprecision(2) << (so.score / 100.0);
        score_str = score_ss.str();
    }
    set_red_bold(m_score_label_value, score_str);
    set_red_bold(m_time_label_value, format_time(so.time_spent));
    
    std::string pv_text = format_pv(so.pv, m_game_state);
    m_pv_text_buffer->set_text(pv_text);

    return true; 
}

bool MainWindow::check_game_over_state() {
    if (m_in_setup_mode) return false;

    auto enviar_dxp_game_end = [this](char reason) {
        if (m_dxp_client.is_conectado()) m_dxp_client.enviar_game_end(reason, '0');
    };

    // 1. Verificar vitória por falta de peças
    if (m_game_state.obter_todas_pecas_brancas() == 0) {
        if (m_minha_cor_dxp == DamasCore::GamePlayer::BRANCAS) enviar_dxp_game_end('1');
        show_game_over_dialog("Vitória das Pretas!\nAs Brancas ficaram sem peças.");
        return true;
    }
    if (m_game_state.obter_todas_pecas_pretas() == 0) {
        if (m_minha_cor_dxp == DamasCore::GamePlayer::PRETOS) enviar_dxp_game_end('1');
        show_game_over_dialog("Vitória das Brancas!\nAs Pretas ficaram sem peças.");
        return true;
    }

    // 2. Verificar vitória por falta de movimentos
    bool has_moves = false;
    auto caps = DamasCore::gerar_todas_capturas_maximais(m_game_state);
    if (!caps.empty()) {
        has_moves = true;
    } else {
        DamasCore::BoardBitboard pieces = (m_game_state.obter_turno_atual() == DamasCore::GamePlayer::BRANCAS) ? m_game_state.obter_todas_pecas_brancas() : m_game_state.obter_todas_pecas_pretas();
        while (pieces) {
            int sq = __builtin_ctzll(pieces);
            bool is_pawn = ((m_game_state.obter_brancas_peoes() | m_game_state.obter_pretas_peoes()) & (1ULL << sq));
            DamasCore::BoardBitboard moves = is_pawn ? DamasCore::gerar_movimentos_simples_peao(m_game_state, sq) : DamasCore::gerar_movimentos_simples_dama(m_game_state, sq);
            if (moves) { has_moves = true; break; }
            pieces &= pieces - 1;
        }
    }

    if (!has_moves) {
        if (m_game_state.obter_turno_atual() == DamasCore::GamePlayer::BRANCAS) {
            if (m_minha_cor_dxp == DamasCore::GamePlayer::BRANCAS) enviar_dxp_game_end('1');
            show_game_over_dialog("Vitória das Pretas!\nAs Brancas não possuem movimentos válidos (Bloqueio).");
        } else {
            if (m_minha_cor_dxp == DamasCore::GamePlayer::PRETOS) enviar_dxp_game_end('1');
            show_game_over_dialog("Vitória das Brancas!\nAs Pretas não possuem movimentos válidos (Bloqueio).");
        }
        return true;
    }

    // 3. Regra de Empate: 20 meios-movimentos (10 lances)
    if (m_game_state.obter_relogio_meio_movimento() >= 20) {
        enviar_dxp_game_end('2'); // Envia código padrão DXP '2' para empate
        show_game_over_dialog("Empate!\nRegra de 10 lances consecutivos de Damas sem captura ou avanço de peão.");
        return true;
    }

    // 4. Regra de Empate: Repetição de Posição
    int rep_count = 0;
    uint64_t current_hash = m_game_state.obter_hash();
    for (size_t i = 0; i <= m_history_index; ++i) {
        if (m_history_stack[i].state.obter_hash() == current_hash) rep_count++;
    }
    if (rep_count >= 3) {
        enviar_dxp_game_end('2'); // Envia código padrão DXP '2' para empate
        show_game_over_dialog("Empate!\nA mesma posição se repetiu 3 vezes no tabuleiro.");
        return true;
    }

    return false;
}

void MainWindow::show_game_over_dialog(const std::string& message) {
}

void MainWindow::set_palette_tool(PaletteTool tool) {
    m_selected_palette_tool = tool;

    // Força o redesenho de todos os itens da paleta para atualizar o destaque
    m_white_pawn_palette_item.queue_draw();
    m_black_pawn_palette_item.queue_draw();
    m_white_king_palette_item.queue_draw();
    m_black_king_palette_item.queue_draw();
    m_eraser_palette_item.queue_draw();
}

bool MainWindow::on_key_press_event(GdkEventKey* key_event)
{
    // Verifica se o modo de montagem não está ativo
    if (!m_in_setup_mode) {
        // Atalhos de teclado para os modos de jogo
        if (key_event->keyval == GDK_KEY_a) {
            if (m_hum_vs_hum_item) m_hum_vs_hum_item->activate();
            return true; // Evento tratado
        } else if (key_event->keyval == GDK_KEY_s) {
            if (m_hum_vs_ai_item) m_hum_vs_ai_item->activate();
            return true; // Evento tratado
        } else if (key_event->keyval == GDK_KEY_d) {
            if (m_ai_vs_hum_item) m_ai_vs_hum_item->activate();
            return true; // Evento tratado
        } else if (key_event->keyval == GDK_KEY_f) {
            if (m_ai_vs_ai_item) m_ai_vs_ai_item->activate();
            return true; // Evento tratado
        } else if (key_event->keyval == GDK_KEY_g) {
            if (m_analysis_item) m_analysis_item->activate();
            return true; // Evento tratado
        }
        // Atalhos para Voltar/Avançar
        if (key_event->keyval == GDK_KEY_Left) {
            on_undo_activated();
            return true; // Evento tratado
        } else if (key_event->keyval == GDK_KEY_Right) {
            on_redo_activated();
            return true; // Evento tratado
        }
    }
    // Se não for uma das teclas que nos interessa, passa o evento para o handler padrão.
    return Gtk::Window::on_key_press_event(key_event);
}

void MainWindow::on_undo_activated() {
    if (m_ai_chain_timer_conn) {
        m_ai_chain_timer_conn.disconnect();
    }
    if (m_is_ai_turn_active) {
        m_discard_next_ai_move = true;
        DamasCore::interrupt_search();
    }
    if (m_history_index > 0) {
        m_last_undone_state = m_game_state; // Fotografa o estado (ruim) antes do Undo para fins de aprendizado de máquina
        m_history_index--;
        const auto* history_ptr = m_history_stack;
        m_game_state = history_ptr[m_history_index].state; // Pega o estado do histórico
        m_selected_square = -1;
        m_last_move_from_sq = history_ptr[m_history_index].last_move_from_sq;
        m_last_move_to_sq = history_ptr[m_history_index].last_move_to_sq;
        m_possible_moves_bitboard = 0;
        m_possible_capture_moves.clear();
        m_saved_win_pv.clear(); // Reseta os lances previstos caso o usuário altere o passado
        m_board_grid.queue_draw();
        update_history_text_view(); // Adicionado para atualizar o histórico
        update_undo_redo_sensitivity();
        log_debug_info();
        
        if (!check_game_over_state() || m_game_mode == GameMode::ANALYSIS) {
            check_for_ai_move();
        }
    }
}

void MainWindow::on_redo_activated() {
    if (m_ai_chain_timer_conn) {
        m_ai_chain_timer_conn.disconnect();
    }
    if (m_is_ai_turn_active) {
        m_discard_next_ai_move = true;
        DamasCore::interrupt_search();
    }
    if (m_history_index < m_history_count - 1) {
        m_history_index++;
        const auto* history_ptr = m_history_stack;
        m_game_state = history_ptr[m_history_index].state; // Pega o estado do histórico
        m_selected_square = -1;
        m_last_move_from_sq = history_ptr[m_history_index].last_move_from_sq;
        m_last_move_to_sq = history_ptr[m_history_index].last_move_to_sq;
        m_possible_moves_bitboard = 0;
        m_possible_capture_moves.clear();
        m_saved_win_pv.clear();
        m_board_grid.queue_draw();
        update_history_text_view(); // Adicionado para atualizar o histórico
        update_undo_redo_sensitivity();
        log_debug_info();
        
        if (!check_game_over_state() || m_game_mode == GameMode::ANALYSIS) {
            check_for_ai_move();
        }
    }
}

void MainWindow::record_new_state(const std::string& move_notation) {
    // Se o índice atual não está no final da pilha (ou seja, o usuário desfez alguns lances),
    // apaga o "futuro" que foi desfeito antes de adicionar o novo estado.
    if (m_history_index < m_history_count - 1) {
        m_history_count = m_history_index + 1;
    }
    if (m_history_count < 2048) {
        m_history_stack[m_history_count++] = {m_game_state, move_notation, m_last_move_from_sq, m_last_move_to_sq};
    }
    m_history_index++;
    update_history_text_view(); // Atualiza a visualização do histórico
    update_undo_redo_sensitivity();
    log_debug_info();
}

void MainWindow::update_history_text_view() {
    auto buffer = m_history_text_view.get_buffer();
    
    std::stringstream ss;
    int move_num = 1;

    // Encontra o índice do primeiro movimento real do jogo
    size_t start_idx = 0;
    bool game_has_started = false;
    const auto* history_ptr = m_history_stack;
    // Começa de 1, pois o índice 0 é sempre uma configuração inicial
    for (size_t i = 0; i < m_history_count; ++i) { 
        const auto& notation = history_ptr[i].move_notation;
            // Verifica se a notação representa um movimento real (contém '-' ou 'x')
            if (notation.find('-') != std::string::npos || notation.find('x') != std::string::npos) {
            start_idx = i;
            game_has_started = true;
            break;
        }
    }

    // Se não há movimentos reais na parte visível do histórico ou se o índice está antes do primeiro movimento real
    if (!game_has_started || m_history_index < start_idx) {
        buffer->set_text(""); // Limpa o texto
        auto end_iter = buffer->end();
        m_history_text_view.scroll_to(end_iter);
        return;
    }

    // Determina quem fez o primeiro lance. O estado em start_idx é *após* o lance.
    DamasCore::GamePlayer player_who_made_first_game_move = 
        (history_ptr[start_idx].state.obter_turno_atual() == DamasCore::GamePlayer::BRANCAS) ? DamasCore::GamePlayer::PRETOS : DamasCore::GamePlayer::BRANCAS;

    size_t current_move_idx = start_idx;

    if (player_who_made_first_game_move == DamasCore::GamePlayer::PRETOS) {
        ss << move_num << ". ... " << history_ptr[current_move_idx].move_notation << "\n";
        move_num++;
        current_move_idx++;
    }

    // Itera sobre o resto dos lances em pares (Brancas, depois Pretas)
    while (current_move_idx <= m_history_index) {
        // Lance das Brancas
        ss << move_num << ". " << history_ptr[current_move_idx].move_notation;
        current_move_idx++;

        // Verifica se há um lance das Pretas para adicionar na mesma linha
        if (current_move_idx <= m_history_index) {
            ss << " " << history_ptr[current_move_idx].move_notation;
            current_move_idx++;
        }
        
        ss << "\n"; // Nova linha após o par (ou após o lance das Brancas se for o último)
        move_num++;
    }

    buffer->set_text(ss.str());
    // Rola para o final do texto
    auto end_iter = buffer->end();
    m_history_text_view.scroll_to(end_iter);
}

std::string MainWindow::get_algebraic_notation(int from_sq, int to_sq, DamasCore::BoardBitboard captured_mask) {
    std::string notation = "";

    notation += (char)('a' + (from_sq % 8));
    notation += (char)('8' - (from_sq / 8));

    notation += (captured_mask != 0) ? "x" : "-";

    notation += (char)('a' + (to_sq % 8));
    notation += (char)('8' - (to_sq / 8));

    return notation;
}

void MainWindow::update_undo_redo_sensitivity() {
    if (m_undo_item) {
        m_undo_item->set_sensitive(!m_in_setup_mode && m_history_index > 0);
    }
    if (m_redo_item) {
        m_redo_item->set_sensitive(!m_in_setup_mode && m_history_index < m_history_count - 1);
    }
}

void MainWindow::update_network_status() {
    if (!m_network_status_label || !m_network_status_item) return;
    if (m_dxp_client.is_conectado()) {
        std::string text = "🟢 ";
        if (m_network_game_state == 2) {
            if (m_minha_cor_dxp == DamasCore::GamePlayer::BRANCAS) text += "Brancas";
            else text += "Pretas";
        } else {
            text += "Aguardando Oponente";
        }
        m_network_status_label->set_markup("<span foreground='#00FF00' weight='bold'>" + text + "</span>");
        m_network_status_item->show_all();
    } else {
        m_network_status_label->set_text("");
        m_network_status_item->hide();
    }
}

// --- Handlers para Conexão de Rede DXP ---
void MainWindow::on_connect_activated() {
    Gtk::Dialog dialog("Conectar ao Servidor DXP", *this, true);
    dialog.add_button("Cancelar", Gtk::RESPONSE_CANCEL);
    dialog.add_button("Conectar", Gtk::RESPONSE_OK);
    dialog.set_default_response(Gtk::RESPONSE_OK);

    Gtk::Grid grid;
    grid.set_border_width(10);
    grid.set_row_spacing(5);
    grid.set_column_spacing(10);

    Gtk::Label host_label("URL:");
    Gtk::Entry host_entry;
    host_entry.set_text("127.0.0.1");

    Gtk::Label port_label("Porta:");
    Gtk::Entry port_entry;
    port_entry.set_text("5000");

    grid.attach(host_label, 0, 0, 1, 1);
    grid.attach(host_entry, 1, 0, 1, 1);
    grid.attach(port_label, 0, 1, 1, 1);
    grid.attach(port_entry, 1, 1, 1, 1);

    dialog.get_content_area()->add(grid);
    dialog.show_all();

    if (dialog.run() == Gtk::RESPONSE_OK) {
        std::string host = host_entry.get_text();
        int port = 5000;
        try { port = std::stoi(port_entry.get_text()); } catch (...) {}

        // Configura os callbacks do protocolo DXP antes de conectar
        m_dxp_client.set_on_mensagem_recebida([](const std::string& /*msg*/) {
        });

        m_dxp_client.set_on_game_req([this](const DamasCore::DXPGameReq& req) {
            // '0' significa que aceitamos o jogo e estamos prontos!
            m_dxp_client.enviar_game_acc("Patriotas", '0'); 
            
            Glib::signal_idle().connect([this, req]() {
                m_minha_cor_dxp = (req.follower_color == 'W') ? DamasCore::GamePlayer::BRANCAS : DamasCore::GamePlayer::PRETOS;
                m_network_game_state = 2; // Partida inciada
                update_network_status();
                DamasCore::GamePlayer minha_cor = (req.follower_color == 'W') ? DamasCore::GamePlayer::BRANCAS : DamasCore::GamePlayer::PRETOS;
                
                if (req.starting_position == 'A') {
                    m_game_state.configurar_posicao_inicial();
                } else if (req.starting_position == 'B') {
                    DamasCore::BoardBitboard wm = 0, bm = 0, wk = 0, bk = 0;
                    DamasCore::GamePlayer turno = (req.color_to_move_first == 'W') ? DamasCore::GamePlayer::BRANCAS : DamasCore::GamePlayer::PRETOS;
                    const char* pos_ptr = req.position.data();
                    size_t pos_len = req.position.length();
                    for (size_t i = 0; i < 32 && i < pos_len; ++i) {
                        int r = i / 4;
                        int f = (i % 4) * 2;
                        if ((r % 2) == 0) f += 1;
                        int sq = r * 8 + f;
                        char p = pos_ptr[i];
                        if (p == 'w' || p == '1') wm |= (1ULL << sq);
                        else if (p == 'b' || p == '2') bm |= (1ULL << sq);
                        else if (p == 'W' || p == '3') wk |= (1ULL << sq);
                        else if (p == 'B' || p == '4') bk |= (1ULL << sq);
                    }
                    m_game_state.definir_posicao_via_bitboards(wm, bm, wk, bk, turno);
                }
                
                reset_game_history(m_game_state, "");
                check_for_ai_move();
                
                return false;
            });
        });

        m_dxp_client.set_on_move([this](const DamasCore::DXPMove& m) {
            Glib::signal_idle().connect([this, m]() {
                auto de_dxp = [](int std) {
                    int n = std - 1;
                    int r = n / 4;
                    int f = (n % 4) * 2;
                    if ((r % 2) == 0) f += 1;
                    return r * 8 + f;
                };
                
                int from_sq = de_dxp(m.from_sq);
                int to_sq = de_dxp(m.to_sq);
                DamasCore::BoardBitboard cap_mask = 0;
                const int* cap_ptr = m.captured_sqs;
                size_t cap_size = m.num_captures;
                for (size_t i = 0; i < cap_size; ++i) cap_mask |= (1ULL << de_dxp(cap_ptr[i]));
                
                DamasCore::GameMove oponente_move{from_sq, to_sq, cap_mask};
                
                if (oponente_move.mascaras_pecas_capturadas != 0) m_game_state.aplicar_movimento_completo_captura(oponente_move);
                else m_game_state.aplicar_mov_simples(oponente_move.casa_origem, oponente_move.casa_destino);
                
                m_game_state.alternar_turno();
                std::string move_notation = get_algebraic_notation(oponente_move.casa_origem, oponente_move.casa_destino, oponente_move.mascaras_pecas_capturadas);
                m_last_move_from_sq = oponente_move.casa_origem;
                m_last_move_to_sq = oponente_move.casa_destino;
                
                record_new_state(move_notation);
                play_move_sound(); // Toca o som do movimento de rede
                m_board_grid.queue_draw();
                if (!check_game_over_state()) {
                    check_for_ai_move();
                }
                
                return false;
            });
        });

        m_dxp_client.set_on_game_end([this](char reason, char stop_code) {
            int resultado = 0; // 1 (Vitória), -1 (Derrota), 0 (Empate)
            if (reason == '1') resultado = 1; // O oponente enviou '1' (You win), então nós ganhamos
            else if (reason == '2') resultado = 0; // O oponente enviou '2' (Draw)
            else if (reason == '0') resultado = -1; // Oponente enviou '0' (I win), nós perdemos
            
            // Atualiza os pesos da IA local via DXP
            m_dxp_client.atualizar_pesos_aprendizado(resultado);
            std::cout << "Patriotas atualizou seus pesos de aprendizado (" << ((resultado > 0) ? "+1 Vitoria" : "-1 Derrota") << ")!" << std::endl;

            // Se o servidor avisar que não haverá mais jogos (stop_code == '1'), desconecta
            if (stop_code == '1') {
                Glib::signal_idle().connect([this]() {
                    on_disconnect_activated();
                    return false;
                });
            } else {
                Glib::signal_idle().connect([this]() {
                    m_network_game_state = 1; // Volta a aguardar o próximo oponente na sala
                    update_network_status();
                    return false;
                });
            }
        });

        // Trata visualmente quando a conexão cai de forma inesperada
        m_dxp_client.set_on_conexao_perdida([this]() {
            Glib::signal_idle().connect([this]() {
                m_connect_item->set_sensitive(true);
                m_disconnect_item->set_sensitive(false);
                m_network_game_state = 0;
                update_network_status();
                return false;
            });
        });

        if (m_dxp_client.conectar(host, port)) {
            m_connect_item->set_sensitive(false);
            m_disconnect_item->set_sensitive(true);
            m_network_game_state = 1; // Aguardando
            update_network_status();
            
            // Só na conexão com o servidor é que o modo volta ao padrão Humano vs Humano
            if (m_hum_vs_hum_item) m_hum_vs_hum_item->set_active(true);
            set_game_mode(GameMode::HUM_VS_HUM);
        } else {
            Gtk::MessageDialog err_dialog(*this, "Falha na Conexão", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
            err_dialog.set_secondary_text("Verifique se o servidor DXP está rodando e se a Porta está correta.");
            err_dialog.run();
        }
    }
}

void MainWindow::on_disconnect_activated() {
    m_dxp_client.desconectar();
    m_connect_item->set_sensitive(true);
    m_disconnect_item->set_sensitive(false);
    m_network_game_state = 0;
    update_network_status();
}

// --- Implementação de FEN/PDN e Lote ---

std::string MainWindow::runFileDialog(const Glib::ustring& title, Gtk::FileChooserAction action) {
    Gtk::FileChooserDialog dialog(*this, title, action);
    dialog.add_button("_Cancelar", Gtk::RESPONSE_CANCEL);
    dialog.add_button(action == Gtk::FILE_CHOOSER_ACTION_SAVE ? "_Salvar" : "_Abrir", Gtk::RESPONSE_OK);

    // Habilita apagar o arquivo selecionado usando a tecla 'Delete'
    dialog.signal_key_press_event().connect([&dialog](GdkEventKey* event) -> bool {
        if (event->keyval == GDK_KEY_Delete) {
            std::string selected_file = dialog.get_filename();
            if (!selected_file.empty() && std::filesystem::is_regular_file(selected_file)) {
                // Usa o GIO para enviar o arquivo para a lixeira do sistema de forma segura
                std::string cmd = "gio trash \"" + selected_file + "\" 2>/dev/null";
                if (std::system(cmd.c_str()) != 0) {
                    // Se falhar (sistema sem lixeira), apaga diretamente
                    std::filesystem::remove(selected_file);
                }
                // Recarrega a pasta para a interface atualizar e o arquivo sumir
                dialog.set_current_folder(dialog.get_current_folder());
                return true; // Evento tratado
            }
        }
        return false; // Permite o fluxo normal de outras teclas
    }, false);

    std::filesystem::path saveDir = std::filesystem::absolute("Salve");
    try {
        if (!std::filesystem::exists(saveDir)) {
            std::filesystem::path parentSave = std::filesystem::absolute("../Salve");
            if (std::filesystem::exists(parentSave)) {
                saveDir = parentSave;
            } else {
                std::filesystem::create_directories(saveDir);
            }
        }
        saveDir = std::filesystem::canonical(saveDir);
    } catch (...) {}
    dialog.set_current_folder(saveDir.string());
    
    int result = dialog.run();
    if (result == Gtk::RESPONSE_OK) return dialog.get_filename();
    return "";
}

std::string MainWindow::generateFEN() {
    std::stringstream ss;
    ss << (m_game_state.obter_turno_atual() == DamasCore::GamePlayer::BRANCAS ? "W" : "B");
    
    std::vector<std::string> whitePieces;
    std::vector<std::string> blackPieces;
    
    auto add_pieces = [&](DamasCore::BoardBitboard bb, bool is_white, bool is_king) {
        while (bb != 0) {
            int sq = __builtin_ctzll(bb);
            std::string coord = "";
            coord += (char)('a' + (sq % 8));
            coord += (char)('8' - (sq / 8));
            if (is_king) coord = "K" + coord;
            
            if (is_white) whitePieces.push_back(coord);
            else blackPieces.push_back(coord);
            
            bb &= bb - 1;
        }
    };
    
    add_pieces(m_game_state.obter_brancas_peoes(), true, false);
    add_pieces(m_game_state.obter_brancas_damas(), true, true);
    add_pieces(m_game_state.obter_pretas_peoes(), false, false);
    add_pieces(m_game_state.obter_pretas_damas(), false, true);
    
    ss << ":W";
    for (size_t i = 0; i < whitePieces.size(); ++i) {
        ss << whitePieces[i] << (i < whitePieces.size() - 1 ? "," : "");
    }
    
    ss << ":B";
    for (size_t i = 0; i < blackPieces.size(); ++i) {
        ss << blackPieces[i] << (i < blackPieces.size() - 1 ? "," : "");
    }
    
    return ss.str();
}

void MainWindow::setBoardFromFEN(const std::string& fen) {
    m_game_state.limpar_tabuleiro();
    
    std::string processedFen = fen;
    size_t tagStart = fen.find("[FEN \"");
    if (tagStart != std::string::npos) {
        tagStart += 6;
        size_t tagEnd = fen.find("\"]", tagStart);
        if (tagEnd != std::string::npos) {
            processedFen = fen.substr(tagStart, tagEnd - tagStart);
        }
    }
    
    size_t colPos = processedFen.find(':');
    if (colPos == std::string::npos) return;
    std::string turnStr = processedFen.substr(0, colPos);
    m_game_state.definir_turno_atual(turnStr == "W" ? DamasCore::GamePlayer::BRANCAS : DamasCore::GamePlayer::PRETOS);
    
    size_t nextCol = processedFen.find(':', colPos + 1);
    if (nextCol == std::string::npos) return;
    std::string whitePart = processedFen.substr(colPos + 1, nextCol - (colPos + 1));
    std::string blackPart = processedFen.substr(nextCol + 1);
    
    auto parsePieces = [&](std::string part, bool is_white) {
        if (part.length() < 2 || (part[0] != 'W' && part[0] != 'B')) return;
        part = part.substr(1);
        std::stringstream ss(part);
        std::string segment;
        while (std::getline(ss, segment, ',')) {
            if (segment.empty()) continue;
            bool isKing = false;
            if (segment[0] == 'K') { isKing = true; segment = segment.substr(1); }
            if (segment.length() < 2) continue;
            int file_coord = segment[0] - 'a';
            int rank_coord = '8' - segment[1];
            if (file_coord < 0 || file_coord > 7 || rank_coord < 0 || rank_coord > 7) continue;
            int sq = rank_coord * 8 + file_coord;
            DamasCore::BoardBitboard mask = (1ULL << sq);
            
            if (is_white) {
                if (isKing) m_game_state.adicionar_brancas_damas(mask);
                else m_game_state.adicionar_brancas_peoes(mask);
            } else {
                if (isKing) m_game_state.adicionar_pretas_damas(mask);
                else m_game_state.adicionar_pretas_peoes(mask);
            }
        }
    };
    
    parsePieces(whitePart, true);
    parsePieces(blackPart, false);
    
    m_game_state.recalcular_hash_completo();
}

void MainWindow::saveFEN(const std::string& filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << "[FEN \"" << generateFEN() << "\"]";
        file.close();
    }
}

void MainWindow::loadFEN(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    std::getline(file, line);
    file.close();

    setBoardFromFEN(line);
    reset_game_history(m_game_state, line);
    check_for_ai_move();
}

std::string MainWindow::getCurrentDate() {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y.%m.%d", std::localtime(&t));
    return std::string(buf);
}

void MainWindow::savePDN(const std::string& filename, bool saveHistory) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    file << "[Event \"Patriotas Game\"]\n";
    file << "[Date \"" << getCurrentDate() << "\"]\n";
    file << "[Result \"*\"]\n";
    
    if (saveHistory) {
        std::string fen = "";
        if (m_history_count > 0) {
            DamasCore::BoardState initial = m_history_stack[0].state;
            DamasCore::BoardState current = m_game_state;
            m_game_state = initial;
            fen = generateFEN();
            m_game_state = current;
        }
        file << "[FEN \"" << fen << "\"]\n";
    } else {
        file << "[FEN \"" << generateFEN() << "\"]\n";
    }
    file << "\n";
    
    if (saveHistory) {
        int move_num = 1;
        for (size_t i = 1; i < m_history_count; ++i) {
            std::string notation = m_history_stack[i].move_notation;
            // Ignora qualquer coisa que não seja um movimento real
            if (notation.find('-') == std::string::npos && notation.find('x') == std::string::npos) continue;
            
            DamasCore::GamePlayer player = m_history_stack[i-1].state.obter_turno_atual();
            
            if (player == DamasCore::GamePlayer::BRANCAS) {
                file << move_num << ". " << notation << " ";
            } else {
                if (i == 1 || (i > 1 && m_history_stack[i-2].state.obter_turno_atual() != DamasCore::GamePlayer::BRANCAS)) { 
                    file << move_num << ". ... " << notation << " ";
                } else {
                    file << notation << " ";
                }
                move_num++;
            }
        }
        file << "*\n";
    }
    file.close();
}

bool MainWindow::applyMoveStr(const std::string& moveStr) {
    DamasCore::MoveList captures = DamasCore::gerar_todas_capturas_maximais(m_game_state);
    
    if (!captures.empty()) {
        for (size_t i = 0; i < captures.size(); ++i) {
            const auto& mv = captures[i];
            std::string notation = get_algebraic_notation(mv.casa_origem, mv.casa_destino, mv.mascaras_pecas_capturadas);
            if (notation == moveStr) {
                m_game_state.aplicar_movimento_completo_captura(mv);
                m_game_state.alternar_turno();
                return true;
            }
        }
    } else {
        DamasCore::BoardBitboard pecas = (m_game_state.obter_turno_atual() == DamasCore::GamePlayer::BRANCAS) ? m_game_state.obter_todas_pecas_brancas() : m_game_state.obter_todas_pecas_pretas();
        while (pecas != 0) {
            int sq = __builtin_ctzll(pecas);
            DamasCore::BoardBitboard mask = (1ULL << sq);
            bool is_pawn = ((m_game_state.obter_brancas_peoes() | m_game_state.obter_pretas_peoes()) & mask) != 0;
            DamasCore::BoardBitboard dests = is_pawn ? DamasCore::gerar_movimentos_simples_peao(m_game_state, sq) : DamasCore::gerar_movimentos_simples_dama(m_game_state, sq);
            
            while (dests != 0) {
                int to_sq = __builtin_ctzll(dests);
                std::string notation = get_algebraic_notation(sq, to_sq, 0);
                if (notation == moveStr) {
                    m_game_state.aplicar_mov_simples(sq, to_sq);
                    m_game_state.alternar_turno();
                    return true;
                }
                dests &= dests - 1;
            }
            pecas &= pecas - 1;
        }
    }
    return false;
}

void MainWindow::loadPDN(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line, content;
    std::string start_fen = "";

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line.rfind("[FEN \"", 0) == 0) {
            start_fen = line.substr(6, line.length() - 8);
        } else if (line[0] != '[') {
            content += line + " ";
        }
    }
    file.close();

    if (!start_fen.empty()) {
        setBoardFromFEN(start_fen);
        reset_game_history(m_game_state, start_fen);
    } else {
        DamasCore::BoardState initial; initial.configurar_posicao_inicial();
        reset_game_history(initial, "Posição Inicial (12x12)");
    }

    std::replace(content.begin(), content.end(), '.', ' ');
    std::stringstream ss(content);
    std::string token;
    while (ss >> token) {
        if (isdigit(token[0]) && token.find('-') == std::string::npos && token.find('x') == std::string::npos) continue; 
        if (token == "*" || token == "1-0" || token == "0-1" || token == "1/2-1/2") break;
        
        std::string move_to_apply = token;
        std::replace(move_to_apply.begin(), move_to_apply.end(), ':', 'x');
        
        int from_sq = -1, to_sq = -1;
        size_t sep_pos = move_to_apply.find('-');
        if (sep_pos == std::string::npos) sep_pos = move_to_apply.find('x');
        if (sep_pos != std::string::npos && sep_pos >= 2 && sep_pos + 3 <= move_to_apply.length()) {
            std::string from_str = move_to_apply.substr(sep_pos - 2, 2);
            std::string to_str = move_to_apply.substr(sep_pos + 1, 2);
            int fc = from_str[0] - 'a'; int fr = '8' - from_str[1];
            int tc = to_str[0] - 'a';   int tr = '8' - to_str[1];
            if (fc>=0 && fc<8 && fr>=0 && fr<8) from_sq = fr * 8 + fc;
            if (tc>=0 && tc<8 && tr>=0 && tr<8) to_sq = tr * 8 + tc;
        }

        if (applyMoveStr(move_to_apply)) {
            m_last_move_from_sq = from_sq;
            m_last_move_to_sq = to_sq;
            record_new_state(move_to_apply);
        }
    }
    m_board_grid.queue_draw();
    if (!check_game_over_state() || m_game_mode == GameMode::ANALYSIS) {
        check_for_ai_move();
    }
}

void MainWindow::importBatchFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;
    m_batch_positions.clear();
    m_batch_labels.clear();
    std::string line;
    int line_num = 0;
    while (std::getline(file, line)) {
        line_num++;
        std::string fen_str;
        size_t fen_tag_start = line.find("[FEN \"");
        if (fen_tag_start != std::string::npos) {
            size_t fen_tag_end = line.find("\"]", fen_tag_start + 6);
            if (fen_tag_end != std::string::npos) {
                fen_str = line.substr(fen_tag_start + 6, fen_tag_end - (fen_tag_start + 6));
            }
        } else {
            if (line.rfind("W:", 0) == 0 || line.rfind("B:", 0) == 0) {
                fen_str = line;
            }
        }

        if (!fen_str.empty()) {
            try {
                DamasCore::BoardState temp_state;
                DamasCore::BoardState current = m_game_state;
                setBoardFromFEN(fen_str);
                temp_state = m_game_state;
                m_game_state = current; 
                
                m_batch_positions.push_back(temp_state);
                std::string label = std::to_string(line_num) + ": " + fen_str;
                if (label.length() > 80) label = label.substr(0, 77) + "...";
                m_batch_labels.push_back(label);
            } catch (const std::exception& e) {
                std::cerr << "Erro ao parsear FEN na linha " << line_num << ": " << e.what() << std::endl;
            }
        }
    }
    file.close();
}

void MainWindow::showBatchSelectionDialog() {
    Gtk::Dialog dialog("Selecionar Posição (Total: " + std::to_string(m_batch_positions.size()) + ")", *this, true);
    dialog.set_default_size(700, 450);

    Glib::RefPtr<Gtk::ListStore> refTreeModel = Gtk::ListStore::create(m_batchColumns);
    Gtk::TreeView treeView;
    treeView.set_model(refTreeModel);
    
    treeView.append_column("Nº", m_batchColumns.m_col_index);
    treeView.append_column("FEN", m_batchColumns.m_col_fen);

    for (size_t i = 0; i < m_batch_labels.size(); ++i) {
        Gtk::TreeModel::Row row = *(refTreeModel->append());
        row[m_batchColumns.m_col_index] = i + 1;
        row[m_batchColumns.m_col_fen] = m_batch_labels[i];
    }

    Gtk::ScrolledWindow scrolledWindow;
    scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scrolledWindow.add(treeView);

    dialog.get_content_area()->pack_start(scrolledWindow, Gtk::PACK_EXPAND_WIDGET);
    
    dialog.add_button("_Cancelar", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Abrir", Gtk::RESPONSE_OK);
    
    dialog.show_all_children();

    treeView.signal_row_activated().connect([&dialog](const Gtk::TreeModel::Path&, Gtk::TreeViewColumn*) {
        dialog.response(Gtk::RESPONSE_OK);
    });

    int result = dialog.run(); 
    if (result == Gtk::RESPONSE_OK) {
        Glib::RefPtr<Gtk::TreeSelection> refSelection = treeView.get_selection();
        if (Gtk::TreeModel::iterator iter = refSelection->get_selected()) {
            Gtk::TreeModel::Row row = *iter;
            int index = row[m_batchColumns.m_col_index] - 1;
            
            if (index >= 0 && index < m_batch_positions.size()) {
                m_game_state = m_batch_positions[index];
                Glib::ustring fen_ustr = row[m_batchColumns.m_col_fen];
                std::string notation_name = fen_ustr;
                reset_game_history(m_game_state, notation_name);
                check_for_ai_move();
            }
        }
    }
}

std::string MainWindow::format_nodes(uint64_t nodes) {
    std::string s = std::to_string(nodes);
    int insertPosition = static_cast<int>(s.length()) - 3;
    while (insertPosition > 0) {
        s.insert(insertPosition, ".");
        insertPosition -= 3;
    }
    return s;
}

std::string MainWindow::format_time(double total_seconds) {
    if (total_seconds < 0) return "0.00s";
    std::stringstream ss;
    if (total_seconds >= 60.0) {
        int minutes = static_cast<int>(total_seconds) / 60;
        double seconds = fmod(total_seconds, 60.0);
        ss << minutes << "m " << std::fixed << std::setprecision(2) << seconds << "s";
    } else ss << std::fixed << std::setprecision(2) << total_seconds << "s";
    return ss.str();
}

std::string MainWindow::format_pv(const std::vector<DamasCore::GameMove>& pv, DamasCore::BoardState initial_state) {
    if (pv.empty()) return "";
    std::stringstream ss;
    DamasCore::BoardState temp_state = initial_state;
    int move_number = 1;
    for (size_t i = 0; i < pv.size(); ++i) {
        const auto& move = pv[i];
        if (temp_state.obter_turno_atual() == DamasCore::GamePlayer::BRANCAS) ss << move_number << ". ";
        else if (i == 0) ss << move_number << ". ... ";
        ss << get_algebraic_notation(move.casa_origem, move.casa_destino, move.mascaras_pecas_capturadas);
        if (temp_state.obter_turno_atual() == DamasCore::GamePlayer::PRETOS) { ss << "\n"; move_number++; } 
        else { ss << " "; }
        if (move.mascaras_pecas_capturadas != 0) temp_state.aplicar_movimento_completo_captura(move);
        else temp_state.aplicar_mov_simples(move.casa_origem, move.casa_destino);
        temp_state.alternar_turno();
    }
    return ss.str();
}

// Função para tocar som de movimento usando thread separada para não travar a UI
void MainWindow::play_move_sound() {
    std::thread([]() {
        std::error_code ec_sym;
        std::filesystem::path exe_dir = std::filesystem::read_symlink("/proc/self/exe", ec_sym).parent_path();
        std::string cmd_exe = "aplay -q " + (exe_dir / "audio" / "move.wav").string() + " 2>/dev/null";
        
        int ret = -1;
        if (!ec_sym && std::filesystem::exists(exe_dir / "audio" / "move.wav")) {
            ret = std::system(cmd_exe.c_str());
        }
        
        if (ret != 0) {
            ret = std::system("aplay -q ../audio/move.wav 2>/dev/null");
            if (ret != 0) {
                std::system("aplay -q /audio/move.wav 2>/dev/null || aplay -q audio/move.wav 2>/dev/null");
            }
        }
    }).detach();
}

void MainWindow::print_board_state() {
    std::cout << "\n=== ESTADO DO TABULEIRO ===\n";
    DamasCore::BoardBitboard wp = m_game_state.obter_brancas_peoes();
    DamasCore::BoardBitboard bp = m_game_state.obter_pretas_peoes();
    DamasCore::BoardBitboard wk = m_game_state.obter_brancas_damas();
    DamasCore::BoardBitboard bk = m_game_state.obter_pretas_damas();

    for (int row = 0; row < 8; ++row) {
        std::cout << (8 - row) << " ";
        for (int col = 0; col < 8; ++col) {
            int sq = row * 8 + col;
            uint64_t mask = 1ULL << sq;
            if (wk & mask) std::cout << "W ";
            else if (wp & mask) std::cout << "w ";
            else if (bk & mask) std::cout << "B ";
            else if (bp & mask) std::cout << "b ";
            else std::cout << ". ";
        }
        std::cout << "\n";
    }
    std::cout << "  a b c d e f g h\n";
    std::cout << "Turno: " << (m_game_state.obter_turno_atual() == DamasCore::GamePlayer::BRANCAS ? "Brancas" : "Pretas") << "\n";
    std::cout << "===========================\n";
}

void MainWindow::print_evaluation() {
    int score_rel = DamasCore::evaluate_board(m_game_state);
    int score_abs = (m_game_state.obter_turno_atual() == DamasCore::GamePlayer::BRANCAS) ? score_rel : -score_rel;
    std::cout << "\n=== AVALIACAO DO TABULEIRO ===\n";
    std::cout << "Score Relativo (Vantagem para quem joga): " << score_rel << "\n";
    std::cout << "Score Absoluto (Vantagem para Brancas): " << score_abs << "\n";
    std::cout << "==============================\n";
}

void MainWindow::log_debug_info() {
    if (m_debug_estado_item && m_debug_estado_item->get_active()) print_board_state();
    if (m_debug_avaliar_item && m_debug_avaliar_item->get_active()) print_evaluation();
}

int main(int argc, char* argv[])
{
    auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example.checkers");

    MainWindow window;

    // Mostra a janela e entra no loop principal do GTK.
    return app->run(window);
}
