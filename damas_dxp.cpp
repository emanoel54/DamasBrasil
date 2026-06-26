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

#include "damas_dxp.hpp"
#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/file.h>
#include <fcntl.h>
#include <cassert>
#include <filesystem>
#include <sstream>
#include <random>

namespace DamasCore {

// --- Utilitários de Parsing DXP ---
static std::string encode_int(int n, int size) {
    std::string s;
    s.resize(size);
    for (int i = 0; i < size; i++) {
        s[size - i - 1] = char('0' + n % 10);
        n /= 10;
    }
    return s;
}

static std::string encode_string(const std::string & s, int size) {
    std::string ns = s;
    for (int i = 0; i < size - int(s.size()); i++) ns += ' ';
    return ns;
}

class Scanner {
private:
    const std::string m_string;
    int m_pos {0};
public:
    explicit Scanner(const std::string & s) : m_string{s} {}
    std::string get_field(int size) { std::string s; for (int i = 0; i < size; i++) s += get_char(); return s; }
    std::string get_field() { return get_field(int(m_string.size()) - m_pos); }
    bool eos() const { return m_pos == int(m_string.size()); }
    char get_char() { if (eos()) return '\0'; return m_string[m_pos++]; }
};
// ----------------------------------

DamasDXP::DamasDXP() : _socket_fd(-1), _conectado(false) {}

DamasDXP::~DamasDXP() {
    desconectar();
}

bool DamasDXP::is_conectado() const {
    return _conectado.load();
}

bool DamasDXP::conectar(const std::string& host, int port) {
    desconectar(); // Garante que qualquer thread/socket anterior seja limpo incondicionalmente

    _socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_socket_fd < 0) {
        return false;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Converte endereços IPv4/IPv6 de texto para binário
    if (inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
        close(_socket_fd);
        return false;
    }

    if (connect(_socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(_socket_fd);
        return false;
    }

    _conectado = true;
    
    // Inicia a thread de escuta assíncrona para não travar a UI
    _thread_rede = std::thread(&DamasDXP::loop_escuta, this);
    
    return true;
}

void DamasDXP::desconectar() {
    _conectado = false;
    if (_socket_fd >= 0) {
        shutdown(_socket_fd, SHUT_RDWR); // Interrompe chamadas bloqueantes instantaneamente (como o recv)
        close(_socket_fd);
        _socket_fd = -1;
    }
    if (_thread_rede.joinable()) {
        _thread_rede.join();
    }
    
}

bool DamasDXP::enviar_mensagem(const std::string& msg) {
    if (!_conectado || _socket_fd < 0) return false;
    
    // Adiciona o caractere nulo '\0' no final da mensagem conforme exigido
    // pela maioria dos clientes/servidores DXP (como implementado no socket.cpp do modelo)
    std::string msg_to_send = msg + '\0';
    ssize_t sent = send(_socket_fd, msg_to_send.c_str(), msg_to_send.length(), 0);
    bool success = (sent == static_cast<ssize_t>(msg_to_send.length()));
    return success;
}

bool DamasDXP::enviar_game_req(const std::string& my_name, char follower_color, int time_min, int num_moves) {
    std::string s = "R";
    s += encode_int(1, 2); // versão 1 do DXP
    s += encode_string(my_name, 32);
    s += follower_color;
    s += encode_int(time_min, 3);
    s += encode_int(num_moves, 3);
    s += 'A'; // Posição inicial padrão
    return enviar_mensagem(s);
}

bool DamasDXP::enviar_game_acc(const std::string& my_name, char accept_code) {
    std::string s = "A";
    s += encode_string(my_name, 32);
    s += accept_code;
    return enviar_mensagem(s);
}

bool DamasDXP::enviar_movimento(int time_spent, int from_sq, int to_sq, const int* captures, size_t num_captures) {
    std::string s = "M";
    if (time_spent > 9999) time_spent = 9999;
    if (time_spent < 0) time_spent = 0;
    s += encode_int(time_spent, 4);
    s += encode_int(from_sq, 2);
    s += encode_int(to_sq, 2);
    s += encode_int(num_captures, 2);
    for (size_t i = 0; i < num_captures; ++i) {
        s += encode_int(captures[i], 2);
    }
    return enviar_mensagem(s);
}

bool DamasDXP::enviar_game_end(char reason, char stop_code) {
    std::string s = "E";
    s += reason;
    s += stop_code;
    return enviar_mensagem(s);
}

bool DamasDXP::enviar_chat(const std::string& text) {
    return enviar_mensagem("C" + text);
}

void DamasDXP::set_on_mensagem_recebida(std::function<void(const std::string&)> callback) {
    _on_mensagem_recebida = callback;
}

void DamasDXP::set_on_conexao_perdida(std::function<void()> callback) {
    _on_conexao_perdida = callback;
}

void DamasDXP::set_on_game_req(std::function<void(const DXPGameReq&)> callback) { _on_game_req = callback; }
void DamasDXP::set_on_game_acc(std::function<void(const std::string&, char)> callback) { _on_game_acc = callback; }
void DamasDXP::set_on_move(std::function<void(const DXPMove&)> callback) { _on_move = callback; }
void DamasDXP::set_on_game_end(std::function<void(char, char)> callback) { _on_game_end = callback; }
void DamasDXP::set_on_chat(std::function<void(const std::string&)> callback) { _on_chat = callback; }

void DamasDXP::processar_mensagem_dxp(const std::string& msg) {
    if (msg.empty()) return;
    Scanner scan(msg);
    char header = scan.get_char();

    try {
        switch (header) {
            case 'R': {
                DXPGameReq req;
                scan.get_field(2); // Pula versão
                req.name = scan.get_field(32);
                req.follower_color = scan.get_char();
                req.time_minutes = std::stoi(scan.get_field(3));
                req.num_moves = std::stoi(scan.get_field(3));
                req.starting_position = scan.get_char();
                if (req.starting_position == 'B') {
                    req.color_to_move_first = scan.get_char();
                    req.position = scan.get_field();
                }
                if (_on_game_req) _on_game_req(req);
                break;
            }
            case 'A': {
                std::string name = scan.get_field(32);
                char acc_code = scan.get_char();
                if (_on_game_acc) _on_game_acc(name, acc_code);
                break;
            }
            case 'M': {
                DXPMove m;
                m.time_spent = std::stoi(scan.get_field(4));
                m.from_sq = std::stoi(scan.get_field(2));
                m.to_sq = std::stoi(scan.get_field(2));
                int n_caps = std::stoi(scan.get_field(2));
                m.num_captures = 0;
                for (int i = 0; i < n_caps; i++) {
                    if (m.num_captures < 32) {
                        m.captured_sqs[m.num_captures++] = std::stoi(scan.get_field(2));
                    } else {
                        scan.get_field(2); // Descarta extras para evitar overflow
                    }
                }
                if (_on_move) _on_move(m);
                break;
            }
            case 'E': {
                char reason = scan.get_char();
                char stop_code = scan.get_char();
                if (_on_game_end) _on_game_end(reason, stop_code);
                break;
            }
            case 'C': {
                if (_on_chat) _on_chat(scan.get_field());
                break;
            }
        }
    } catch (...) {
    }
}

void DamasDXP::loop_escuta() {
    char buffer[1024];
    std::string accumulated_data;

    while (_conectado) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_recebidos = recv(_socket_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_recebidos > 0) {
            accumulated_data.append(buffer, bytes_recebidos);

            // Tenta processar caso o servidor envie \0 como delimitador (padrão)
            size_t pos = 0;
            while ((pos = accumulated_data.find('\0')) != std::string::npos) {
                std::string mensagem = accumulated_data.substr(0, pos);
                accumulated_data.erase(0, pos + 1); // Apaga a msg processada do buffer

                if (_on_mensagem_recebida) _on_mensagem_recebida(mensagem);
                processar_mensagem_dxp(mensagem);
            }

            // Para servidores que não usam \0, checamos diretamente se existe uma mensagem
            // completa que faz sentido baseada na primeira letra (R, A, M, E, C).
            if (!accumulated_data.empty() && accumulated_data.find('\0') == std::string::npos) {
                char h = accumulated_data[0];
                bool is_complete = false;
                if (h == 'A' && accumulated_data.length() >= 34) is_complete = true;
                else if (h == 'R' && accumulated_data.length() >= 43) is_complete = true;
                else if (h == 'M' && accumulated_data.length() >= 11) {
                    int num_caps = std::stoi(accumulated_data.substr(9, 2));
                    if (accumulated_data.length() >= static_cast<size_t>(11 + num_caps * 2)) is_complete = true;
                }
                else if (h == 'E' && accumulated_data.length() >= 3) is_complete = true;
                else if (h == 'C') is_complete = true;

                if (is_complete) {
                    if (_on_mensagem_recebida) _on_mensagem_recebida(accumulated_data);
                    processar_mensagem_dxp(accumulated_data);
                    accumulated_data.clear();
                }
            }

        } else if (bytes_recebidos == 0 || (bytes_recebidos < 0 && _conectado)) {
            _conectado = false;
            if (_on_conexao_perdida) {
                _on_conexao_perdida();
            }
            break;
        }
    }
}

// Lista com os nomes das regras para manter os comentarios no arquivo ao regravar
static const char* NOME_REGRAS_IA_DXP[18] = {
    "Material Basico (Peoes e Damas)",
    "Defesa da Fileira de Coroacao (Back Rank)",
    "Peoes Apoiados (Diagonais conectadas)",
    "Pedra Cao (Outposts em c5/f6/c3/f4)",
    "Controle do Centro Expansivo",
    "Controle do Carreirao Principal (a1-h8)",
    "Peoes nas Bordas (Seguros mas sem dominio)",
    "Avanco de Peoes (Incentivo direcional)",
    "Especialistas (Armadilhas e prisoes)",
    "Balanceamento dos Flancos",
    "Estrutura de Defesa Forte do Fundo",
    "Dominio do Centro Absoluto",
    "Penalidade de Estruturas Fracas",
    "Combate aos Postos Inimigos",
    "Desenvolvimento do Carreirao Inferior",
    "Penalidade de Avanco Prematuro (5a fileira)",
    "Mobilidade e Liberdade de Movimentos",
    "Escudos da Base (Triangulos de Protecao)"
};

void DamasDXP::atualizar_pesos_aprendizado(int resultado) {
    std::string arquivo_pesos = "data/pesos_tabela.bin";
    std::string arquivo_memoria = "data/pesos_memoria.bin";
    std::error_code ec_sym;
    std::filesystem::path exe_dir = std::filesystem::read_symlink("/proc/self/exe", ec_sym).parent_path();

    if (!ec_sym && std::filesystem::exists(exe_dir / "data")) {
        arquivo_pesos = (exe_dir / "data" / "pesos_tabela.bin").string();
        arquivo_memoria = (exe_dir / "data" / "pesos_memoria.bin").string();
    } else if (std::filesystem::exists("data")) {
        arquivo_pesos = "data/pesos_tabela.bin";
        arquivo_memoria = "data/pesos_memoria.bin";
    } else if (std::filesystem::exists("../data")) {
        arquivo_pesos = "../data/pesos_tabela.bin";
        arquivo_memoria = "../data/pesos_memoria.bin";
    } else if (std::filesystem::exists("/opt/patriotas/data")) {
        arquivo_pesos = "/opt/patriotas/data/pesos_tabela.bin";
        arquivo_memoria = "/opt/patriotas/data/pesos_memoria.bin";
    } else {
        std::error_code ec;
        if (!ec_sym && !exe_dir.empty()) {
            std::filesystem::create_directories(exe_dir / "data", ec);
            arquivo_pesos = (exe_dir / "data" / "pesos_tabela.bin").string();
            arquivo_memoria = (exe_dir / "data" / "pesos_memoria.bin").string();
        } else {
            std::filesystem::create_directory("data", ec);
        }
    }

    // --- INÍCIO DO BLOQUEIO CROSS-PROCESS ---
    // Garante que múltiplas instâncias do Patriotas não corrompam o arquivo ao salvar juntas
    std::string lock_path = arquivo_pesos + ".lock";
    int fd_lock = open(lock_path.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd_lock >= 0) {
        flock(fd_lock, LOCK_EX); // Fica aguardando se outra instância estiver gravando
    }

    // Lê os pesos (mutações) atuais
    std::vector<int> pesos_atuais;
    std::ifstream file_in(arquivo_pesos, std::ios::binary);
    if (file_in.is_open()) {
        int p;
        while (file_in.read(reinterpret_cast<char*>(&p), sizeof(p))) {
            pesos_atuais.push_back(p);
        }
        file_in.close();
    }
    while (pesos_atuais.size() < 18) pesos_atuais.push_back(100);

    // Lê a memória histórica da melhor configuração até o momento
    std::vector<int> pesos_melhores;
    std::ifstream mem_in(arquivo_memoria, std::ios::binary);
    if (mem_in.is_open()) {
        int p;
        while (mem_in.read(reinterpret_cast<char*>(&p), sizeof(p))) {
            pesos_melhores.push_back(p);
        }
        mem_in.close();
    }
    while (pesos_melhores.size() < 18) pesos_melhores.push_back(pesos_atuais[pesos_melhores.size()]);

    // 1. AVALIA O TESTE ANTERIOR: Verifica se a mutação resultou em Vitória ou Derrota
    if (resultado > 0) {
        pesos_melhores = pesos_atuais; // Vitória: A mutação foi boa! Vira a nova configuração base.
    } else if (resultado == -1 || resultado == 0) {
        pesos_atuais = pesos_melhores; // Derrota ou Empate: Reverte os pesos para a melhor memória segura.
    }
    // Se resultado == -2 (Botão Ruim manual da Interface), NÃO reverte. Deixa acumular a bronca.

    // 2. NOVA MUTAÇÃO: Escolhe 2 parâmetros aleatórios para ajustar e testar na próxima partida
    int num_mutacoes = (resultado == -2) ? 4 : 2; // Mutação em dobro se for aprendizado manual
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist_idx(0, 17);
    std::uniform_int_distribution<int> dist_delta((resultado == -2) ? -15 : -8, (resultado == -2) ? 15 : 8); // Choque mais forte

    for (int i = 0; i < num_mutacoes; i++) {
        int idx = dist_idx(rng);
        int delta = dist_delta(rng);
        if (idx == 0 && resultado != -2) delta /= 2; // O "Material Básico" é muito sensível
        
        pesos_atuais[idx] += delta;
        
        // Limites para a IA não enlouquecer e os cálculos não causarem Overflows no Alpha-Beta
        if (pesos_atuais[idx] < 10) pesos_atuais[idx] = 10;
        if (pesos_atuais[idx] > 400) pesos_atuais[idx] = 400;
    }

    // 3. Salva os arquivos no disco
    std::ofstream file_out(arquivo_pesos, std::ios::binary);
    for (size_t i = 0; i < 18; i++) {
        int p = pesos_atuais[i];
        file_out.write(reinterpret_cast<char*>(&p), sizeof(p));
    }
    file_out.close();

    std::ofstream mem_out(arquivo_memoria, std::ios::binary);
    for (size_t i = 0; i < 18; i++) {
        int p = pesos_melhores[i];
        mem_out.write(reinterpret_cast<char*>(&p), sizeof(p));
    }
    mem_out.close();

    // --- FIM DO BLOQUEIO CROSS-PROCESS ---
    // Libera o arquivo para a próxima instância poder salvar
    if (fd_lock >= 0) {
        flock(fd_lock, LOCK_UN);
        close(fd_lock);
    }
}

} // namespace DamasCore