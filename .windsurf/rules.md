# Regras do Projeto MouseInjectorDolphinDuck

## Personalidade e Estilo
- Sempre use C padrão C99
- Mantenha compatibilidade com Windows (TDM-GCC/MinGW)
- Use indentação com 4 espaços
- Mantenha comentários em português quando possível

## Regras Técnicas
- NÃO modifique arquivos .gitignore
- Prefira fixes minimalistas e upstream
- Use variáveis estáticas para funções específicas de jogos
- Mantenha compatibilidade com a estrutura existente de drivers

## Estrutura do Projeto
- games/: Drivers específicos para cada jogo
- manymouse/: Biblioteca de input (não modificar)
- main.c: Código principal (não modificar)
- memory.c: Funções de memória (não modificar)

## Compilação
- Use makefile existente
- Requer TDM-GCC 4.9.2-tdm-3 ou MinGW
- Arquivo de saída: "Mouse Injector.exe"
- Não adicione dependências externas

## Debugging
- Foco em endereços de memória específicos de jogos
- Teste com emuladores suportados: Dolphin, DuckStation, PCSX2, RetroArch, PPSSPP, Project64
- Verifique compatibilidade de versões de ROMs/ISOs

## Prioridades
1. Não quebre suporte existente
2. Mantenha padrões de código do projeto
3. Teste com jogos listados no README
4. Preserve estrutura de drivers de jogos
