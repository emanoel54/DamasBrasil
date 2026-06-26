#ifndef DAMAS_AI_HPP
#define DAMAS_AI_HPP

#include "damas_core.hpp"
#include <vector>
#include <atomic>
#include <mutex>
#include <array>
#include <chrono>

namespace DamasCore {

// Estrutura de Entrada baseada no Scan
struct Search_Input {
    double time = 1.0; // Tempo em segundos
    int max_depth = 100;
    void init() { time = 1.0; max_depth = 100; }
};

// Estrutura de Saída baseada no Scan
struct Search_Output {
    GameMove move;
    int score = 0;
    int depth = 0;
    uint64_t node = 0;
    double time_spent = 0.0;
    std::vector<GameMove> pv;

    double time() const { return time_spent; }
};

// API Global de Busca do Motor
void search(Search_Output& so, const BoardState& root_node, const Search_Input& si);
void interrupt_search();
void clear_interrupt();
Search_Output get_current_so();

// Funções para Debug
void set_debug_arvore(bool active);
int evaluate_board(const BoardState& pos);

// Exposição da Tabela de Transposição para manipulações de sistema (limpeza)
void clear_hash();

// API de Aprendizado Supervisionado Guiado por Correção
void aprender_com_correcao(const BoardState& estado_bom, const BoardState& estado_ruim);

// Constantes de Pontuação Tática
constexpr int SCORE_MATE = 20000;
constexpr int SCORE_INF = 30000;

// Enum para o tipo de nó na Tabela de Transposição
enum TTFlag { TT_EXACT = 0, TT_ALPHA = 1, TT_BETA = 2 };

} // namespace DamasCore

#endif // DAMAS_AI_HPP