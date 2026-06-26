#ifndef DAMAS_DXP_HPP
#define DAMAS_DXP_HPP

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>

namespace DamasCore {

// --- Estruturas de Dados do Protocolo DXP ---
struct DXPMove {
    int time_spent;
    int from_sq;
    int to_sq;
    int captured_sqs[32];
    size_t num_captures;
};

struct DXPGameReq {
    std::string name;
    char follower_color;
    int time_minutes;
    int num_moves;
    char starting_position;
    char color_to_move_first;
    std::string position;
};
// --------------------------------------------

/**
 * @class DamasDXP
 * @brief Gerencia a conexão com servidores DXP e o protocolo de mensagens.
 */
class DamasDXP {
public:
    DamasDXP();
    ~DamasDXP();

    // Conexão e Desconexão TCP
    bool conectar(const std::string& host, int port);
    void desconectar();
    bool is_conectado() const;
    
    // Envio de mensagens formato DXP (ex: lances)
    bool enviar_mensagem(const std::string& msg);

    // Encoders do Protocolo DXP
    bool enviar_game_req(const std::string& my_name, char follower_color, int time_min, int num_moves);
    bool enviar_game_acc(const std::string& my_name, char accept_code); // '0' = sim
    bool enviar_movimento(int time_spent, int from_sq, int to_sq, const int* captures, size_t num_captures);
    bool enviar_game_end(char reason, char stop_code);
    bool enviar_chat(const std::string& text);

    // Callbacks para integrar com a interface gráfica (GTK) e o jogo
    void set_on_mensagem_recebida(std::function<void(const std::string&)> callback);
    void set_on_conexao_perdida(std::function<void()> callback);

    void set_on_game_req(std::function<void(const DXPGameReq&)> callback);
    void set_on_game_acc(std::function<void(const std::string&, char)> callback);
    void set_on_move(std::function<void(const DXPMove&)> callback);
    void set_on_game_end(std::function<void(char, char)> callback);
    void set_on_chat(std::function<void(const std::string&)> callback);

    // Função básica de Aprendizado por Reforço (Reinforcement Learning Simplificado)
    // resultado: +1 (Vitória), -1 (Derrota), 0 (Empate)
    void atualizar_pesos_aprendizado(int resultado);

private:
    void loop_escuta();
    void processar_mensagem_dxp(const std::string& msg);

    int _socket_fd;
    std::atomic<bool> _conectado;
    std::thread _thread_rede;

    std::function<void(const std::string&)> _on_mensagem_recebida;
    std::function<void()> _on_conexao_perdida;
    std::function<void(const DXPGameReq&)> _on_game_req;
    std::function<void(const std::string&, char)> _on_game_acc;
    std::function<void(const DXPMove&)> _on_move;
    std::function<void(char, char)> _on_game_end;
    std::function<void(const std::string&)> _on_chat;
};

} // namespace DamasCore

#endif // DAMAS_DXP_HPP