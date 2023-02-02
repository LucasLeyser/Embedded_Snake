/*__________________________________________________________________________________
|       Disciplina de Sistemas Embarcados - 2022-1
|       Prof. Douglas Renaux
| __________________________________________________________________________________
|
|       Projeto Final
|       Alunos: Gustavo Zeni 
|               Lucas Perin
| __________________________________________________________________________________
*/

#include <stdint.h>

#include "tx_api.h"
#include "snake_list.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "inc/hw_memmap.h"


#define DEMO_STACK_SIZE         1024
#define DEMO_BYTE_POOL_SIZE     9120
#define DEMO_BLOCK_POOL_SIZE    100
#define DEMO_QUEUE_SIZE         100

#define CENTER_JOY  2000
#define MAX_SPEED   19


TX_THREAD               thread_game;
TX_THREAD               thread_joy;
TX_THREAD               thread_lcd;
TX_THREAD               thread_pause;
TX_QUEUE                joy_updated;
TX_QUEUE                update_lcd;
TX_BYTE_POOL            byte_pool_0;
TX_BLOCK_POOL           block_pool_0;
TX_EVENT_FLAGS_GROUP    pause_flag;
UCHAR                   byte_pool_memory[DEMO_BYTE_POOL_SIZE];

UINT ui32SysClock; //Clock em Hz

extern volatile bool pause; // Variavel de controle do Pause
// X e Y do joystick
extern UINT joy_X;
extern UINT joy_Y;

struct Node *head, *tail, *food;    // Ponteiros para a cabeça e cauda da cobra, assim como a comida atual
float snake_speed = 1.5;            // Inicia em uma posição e meia por segundo
UINT tail_x, tail_y;                // Guarda a posição anterior da cauda da cobra (para apagar no LCD)

extern void initJOY(void); 
extern void updateJOY(void); 
extern void initPAUSE(void);
extern void initSysTick(void);
extern void initLCD(void); 
extern void initBackground(void);
extern void new_print(UINT x, UINT y, UINT flag);
void thread_game_entry(ULONG thread_input);
void thread_joy_entry(ULONG thread_input);
void thread_lcd_entry(ULONG thread_input);
void thread_pause_entry(ULONG thread_input);
extern void pause_IntHandler(void);
extern void print_pause(bool flag);
extern void game_over(int size);
extern void you_win(void);
extern int snake_collision(Node *head);


int main(int argc, char ** argv)
{
    // Seta o clock e inicializa o TreadX
    ui32SysClock = SysCtlClockFreqSet(( SYSCTL_XTAL_25MHZ |
                                        SYSCTL_OSC_MAIN |
                                        SYSCTL_USE_PLL |
                                        SYSCTL_CFG_VCO_240), 120000000);
    tx_kernel_enter();
}   

void tx_application_define(void *first_unused_memory)
{
    char    *pointer = (char*) TX_NULL;
    
    /* Create a byte memory pool from which to allocate the thread stacks.  */
    tx_byte_pool_create(&byte_pool_0, "byte pool 0", byte_pool_memory, DEMO_BYTE_POOL_SIZE);
    
    // Aloca e cria as tarefas
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
    tx_thread_create(&thread_game, "thread game", thread_game_entry, 0, pointer, DEMO_STACK_SIZE, 
                        4, 4, TX_NO_TIME_SLICE, TX_AUTO_START);
    
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
    tx_thread_create(&thread_joy, "thread joy", thread_joy_entry, 0, pointer, DEMO_STACK_SIZE, 
                        3, 3, TX_NO_TIME_SLICE, TX_AUTO_START);
            
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
    tx_thread_create(&thread_lcd, "thread lcd", thread_lcd_entry, 0, pointer, DEMO_STACK_SIZE, 
                        2, 2, TX_NO_TIME_SLICE, TX_AUTO_START);
            
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
    tx_thread_create(&thread_pause, "thread pause", thread_pause_entry, 0, pointer, DEMO_STACK_SIZE, 
                        1, 1, TX_NO_TIME_SLICE, TX_DONT_START);
    
    // Aloca e cria as filas de mensagem
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_QUEUE_SIZE*sizeof(ULONG), TX_NO_WAIT);
    tx_queue_create(&joy_updated, "joy update", TX_1_ULONG, pointer, DEMO_QUEUE_SIZE*sizeof(ULONG));
    
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_QUEUE_SIZE*sizeof(ULONG), TX_NO_WAIT);
    tx_queue_create(&update_lcd, "updated lcd", TX_1_ULONG, pointer, DEMO_QUEUE_SIZE*sizeof(ULONG));
    
    // Cria a event flag que acorda a thread Pause
    tx_event_flags_create(&pause_flag, "flag pause");
    
    /* Allocate the memory for a small block pool.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_BLOCK_POOL_SIZE, TX_NO_WAIT);
        
    /* Create a block memory pool to allocate a message buffer from.  */
    tx_block_pool_create(&block_pool_0, "block pool 0", sizeof(ULONG), pointer, DEMO_BLOCK_POOL_SIZE);
    
    /* Allocate a block and release the block memory.  */
    tx_block_allocate(&block_pool_0, (VOID **) &pointer, TX_NO_WAIT);

    /* Release the block back to the pool.  */
    tx_block_release(pointer);
}

void thread_game_entry(ULONG thread_input)
{
    UINT direction = 1;     // Inicia movendo para a direita
    
    UINT flag = 0;          // Flag = 0 -> Primeira iteração, apenas desenha a cobra
                            // Flag = 1 -> Atualiza a Cobra - NÃO COMEU
                            // Flag = 2 -> Atualiza a Cobra - COMEU
                            
    UINT size_snake = 5;    // Variavel para controle do tamanho
    UINT update = 1;        // Flag para update ou não da cobra no LCD
    
    // Criando a cobra inicial e a primeira comida
    head = snake_create();
    head = snake_add(head, 3, 8, 1);
    tail = head;
    tail_x = tail->x;
    tail_y = tail->y;
    head = snake_add(head, 4, 8, 1);
    head = snake_add(head, 5, 8, 1);
    head = snake_add(head, 6, 8, 1);
    head = snake_add(head, 7, 8, 1);
    food = new_food(head);
    
    // Desenha as estruturas criadas acima no LCD
    tx_queue_send(&update_lcd, &flag, TX_WAIT_FOREVER);
    
    
    while(1)
    {
        while (!pause)
        {
            flag = 1;   // Estado inicial = NÃO COMEU
            tx_queue_receive(&joy_updated, &direction, TX_NO_WAIT); // Atualiza a direção se houver novo input do Joystick
            
            if  (snake_ate(head, food, direction)) //Comeu a comida?
            {
                head = snake_add(head, food->x, food->y, direction); // Adiciona uma posição na cobra, nova cabeça.
                
                if (snake_speed < MAX_SPEED)
                    snake_speed = snake_speed * (float)1.05;    //Aumenta a velocidade (se menor que o limite)
                
                food = new_food(head);  // Nova Comida
                size_snake++;           // Tamanho aumenta em 1
                
                if (size_snake == 196)  // Se tamanho máximo, ganhou o jogo
                {
                    you_win();                
                    snake_free(head); //Desaloca a cobra
                    //Mata as threads
                    tx_thread_terminate(&thread_joy);
                    tx_thread_terminate(&thread_lcd);
                    tx_thread_terminate(&thread_pause);
                    tx_thread_terminate(&thread_game);
                    
                    update = 0;
                }
                flag = 2;   // COMEU
            }
            else  // Se não comeu, guarda a posição da cauda atual (para o LCD apagar) e atualiza a cobra.
            {
                tail_x = tail->x;
                tail_y = tail->y;
                snake_update(head, direction);
            }
            
            if ((((head->x) == 0) || ((head->x) == 15) || ((head->y) == 0) || ((head->y) == 15)) || (snake_collision(head))) //Colisão com as paredes ou com o corpo
            {
                // Colidiu, jogo acaba
                game_over(size_snake);
                snake_free(head);   //Desaloca a cobra
                //Mata as threads
                tx_thread_terminate(&thread_joy);
                tx_thread_terminate(&thread_lcd);
                tx_thread_terminate(&thread_pause);
                tx_thread_terminate(&thread_game);
                    
                update = 0;
            }
            
            if (update) // Se tiver que atualizar o LCD, envia mensagem
                tx_queue_send(&update_lcd, &flag, TX_WAIT_FOREVER);
            
            /*  Dorme a tarefa para simular a velocidade da cobra,
                (1000/snake_speed) é o equivalente a FPS,
                (0.668*size_snake) é uma correção com o tempo aproximado que a 
                verificação de colisão leva conforme a cobra aumenta de tamanho */
            UINT sleep = (UINT)((1000/snake_speed)-(0.668*size_snake));
            tx_thread_sleep(sleep);
        }
    }
}

void thread_joy_entry(ULONG thread_input)
{
    UINT dif_x = 0, dif_y = 0;              // Deslocamento nos dois eixos
    ULONG direction = 1, new_direction = 0; // Direção atual e nova direção lida
    initJOY();
    while(1)
    {
        while (!pause)
        {
            updateJOY();
            /* direction: 0 -> Centro ; 1 -> Direita ; 2 -> Esquerda ; 3 -> Cima ; 4 -> Baixo */
            // Calcula a diferença entre a direção atual e o centro do joystick
            if (joy_X > CENTER_JOY)
                dif_x = joy_X - CENTER_JOY;
            else
                dif_x = CENTER_JOY - joy_X;
            
            if (joy_Y > CENTER_JOY)
                dif_y = joy_Y - CENTER_JOY;
            else
                dif_y = CENTER_JOY - joy_Y;
            
            // Confere se a mudança é relevante ou se não é uma direção oposta à anterior antes de enviar a nova direção lida
            if ((dif_x > dif_y) && (dif_x > (CENTER_JOY/2)) && (direction != 1) && (direction != 2))
            {
                if (joy_X > CENTER_JOY)         // Direita
                    new_direction = 1;
                else if (joy_X < CENTER_JOY)    // Esquerda
                    new_direction = 2;
                
                tx_queue_send(&joy_updated, &new_direction, TX_WAIT_FOREVER);   // Envia a nova direção para a thread do jogo
                
                direction = new_direction; // Atualiza a direção atual
            }
            else if ((dif_y > dif_x) && (dif_y > (CENTER_JOY/2)) && (direction != 3) && (direction != 4))
            {
                if (joy_Y > CENTER_JOY)         // Cima
                    new_direction = 3;
                else if (joy_Y < CENTER_JOY)    // Baixo
                    new_direction = 4;
                
                tx_queue_send(&joy_updated, &new_direction, TX_WAIT_FOREVER);   // Envia a nova direção para a thread do jogo
        
                direction = new_direction;  // Atualiza a direção atual
            }
           
            tx_thread_sleep(25); // Delay entre as leituras
        }
    }
}

void thread_lcd_entry(ULONG thread_input)
{
    UINT COMIDA = 0, COBRA = 1, BACKGROUND = 2; // Flags para "o que o LCD deve desenhar"
    ULONG update = 0;   // Flag de controle do estado do jogo a ser representado
    initLCD();
    tx_thread_resume(&thread_pause); // Com o LCD inicializado, a thread pause pode começar a rodar
    while(1)
    {
        while (!pause)
        {
            // Fica esperando uma nova mensagem para iniciar o update da imagem
            tx_queue_receive(&update_lcd, &update, TX_WAIT_FOREVER);
            
            if (update == 0) // Primeira iteração
            {
                initBackground();       // Desenha o background
                new_print(3, 8, COBRA); // Desenha as 5 posições da cobra, o inicio é sempre fixo
                new_print(4, 8, COBRA);
                new_print(5, 8, COBRA);
                new_print(6, 8, COBRA);
                new_print(7, 8, COBRA);
                new_print(food->x, food->y, COMIDA);    // Desenha a comida
            }
            
            else if (update == 1)   // Atualiza a Cobra quando ela NÃO COMEU
            {
                new_print(head->x, head->y, COBRA);     // Desenha a cabeça na nova posição
                new_print(tail_x, tail_y, BACKGROUND);  // Apaga cauda (última posição), com as variaveis salvas pela thread game
            }  
            
            else if (update == 2)   // Atualiza a Cobra quando ela COMEU
            {
                new_print(food->x, food->y, COMIDA);    // Desenha a nova comida
                new_print(head->x, head->y, COBRA);     // Desenha a nova cabeça da cobra
            } 
        }
    }
}

void thread_pause_entry(ULONG thread_input)
{   
    ULONG flag = 0;
    
    initPAUSE();    
    while(1)
    {
        // Espera a event flag da ISR para executar
        tx_event_flags_get(&pause_flag, 0x1, TX_OR_CLEAR, &flag, TX_WAIT_FOREVER);   
        
        if (pause) // Pausa todas as outras threads
        {
            tx_thread_suspend(&thread_game);
            tx_thread_suspend(&thread_lcd);
            tx_thread_suspend(&thread_joy);
        }
        else    // Acorda todas as outras threads
        {
            tx_thread_resume(&thread_game);
            tx_thread_resume(&thread_lcd);
            tx_thread_resume(&thread_joy);
        }
        // Escreve no LCD o estado atual da variavel pause
        print_pause(pause);
    }
}

