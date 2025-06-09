#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ==== Token infrastructure ====

#define MAX_TOKENS 1024
#define MAX_LEXEME_LEN 64

typedef struct
{
    char type[MAX_LEXEME_LEN];   // e.g., "KEYWORD", "NUM", "OP"
    char lexeme[MAX_LEXEME_LEN]; // e.g., "def", "gcd", "=="
} Token;

Token tokens[MAX_TOKENS];
int token_count = 0;
int current_token = 0;

// Table
#define MAX_SYMBOLS 256
#define MAX_NAME_LEN 64
#define MAX_TYPE_LEN 16
FILE *error_log;
typedef struct
{
    char name[MAX_NAME_LEN];
    char type[MAX_TYPE_LEN];
    char scope[MAX_NAME_LEN]; // e.g., "global", function name, etc.
} Symbol;

Symbol symbol_table[MAX_SYMBOLS];
int symbol_count = 0;
char current_scope[MAX_NAME_LEN] = "global";  // Tracks current scope
char current_decl_type[MAX_TYPE_LEN] = "int"; // Tracks type during declarations
char current_expr_type[MAX_TYPE_LEN] = "";
void insert_symbol(const char *name, const char *type, const char *scope)
{
    // Check for duplicates
    int i;
    for (i = 0; i < symbol_count; ++i)
    {
        if (strcmp(symbol_table[i].name, name) == 0 &&
            strcmp(symbol_table[i].scope, scope) == 0)
        {
            printf("Warning: Redeclaration of '%s' in scope '%s'\n", name, scope);
            return;
        }
    }

    if (symbol_count >= MAX_SYMBOLS)
    {
        fprintf(stderr, "Symbol table overflow\n");
        exit(1);
    }

    strcpy(symbol_table[symbol_count].name, name);
    strcpy(symbol_table[symbol_count].type, type);
    strcpy(symbol_table[symbol_count].scope, scope);

    // printf("[SYMBOL ADDED] name='%s' type='%s' scope='%s'\n",
    // symbol_table[symbol_count].name,
    // symbol_table[symbol_count].type,
    // symbol_table[symbol_count].scope);

    symbol_count++;
}

void print_symbol_table()
{
    printf("\n=== SYMBOL TABLE ===\n");
    printf("%-20s %-10s %-10s\n", "Name", "Type", "Scope");
    printf("-------------------- ---------- ----------\n");
    int i;
    for (i = 0; i < symbol_count; ++i)
    {
        printf("%-20s %-10s %-10s\n", symbol_table[i].name,
               symbol_table[i].type,
               symbol_table[i].scope);
    }
    printf("====================\n\n");
}

void load_tokens(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Cannot open token file");
        exit(1);
    }

    char line[128];
    while (fgets(line, sizeof(line), file) && token_count < MAX_TOKENS)
    {
        char *comma = strchr(line, ',');
        if (!comma)
            continue;
        *comma = '\0';
        strcpy(tokens[token_count].type, line);
        strcpy(tokens[token_count].lexeme, comma + 1);
        size_t len = strlen(tokens[token_count].lexeme);
        if (len > 0 && tokens[token_count].lexeme[len - 1] == '\n')
        {
            tokens[token_count].lexeme[len - 1] = '\0';
        }
        token_count++;
    }

    fclose(file);
}

Token peek_token()
{
    return tokens[current_token];
}

Token next_token()
{
    if (current_token >= token_count)
    {
        printf("Error: Trying to access token beyond bounds\n");
        exit(1);
    }
    Token t = tokens[current_token++];
    // printf("Consuming token: %s,%s\n", t.type, t.lexeme); // Debugging line
    return t;
}

int match_token(const char *expected_type, const char *expected_lexeme)
{

    Token t = peek_token();
    // printf("Matching token: Type = %s, Lexeme = %s\n", t.type, t.lexeme); // Print the current token before matching
    if ((strcmp(expected_type, t.type) == 0 || strcmp(expected_type, "*") == 0) &&
        (strcmp(expected_lexeme, t.lexeme) == 0 || strcmp(expected_lexeme, "*") == 0))
    {
        current_token++;
        return 1;
    }
    return 0;
}

const char *lookup_symbol_type(const char *name, const char *scope)
{
    int i;
    // 1. Exact match in current scope
    for (i = 0; i < symbol_count; ++i)
    {
        if (strcmp(symbol_table[i].name, name) == 0 &&
            strcmp(symbol_table[i].scope, scope) == 0)
            return symbol_table[i].type;
    }

    // 2. If scope is a nested block (e.g., gcd_if_17), fall back to the function scope (e.g., gcd)
    char parent_scope[MAX_NAME_LEN];
    strcpy(parent_scope, scope);
    char *underscore = strchr(parent_scope, '_');
    if (underscore)
    {
        *underscore = '\0'; // chop at first underscore → "gcd_if_17" → "gcd"
        for (i = 0; i < symbol_count; ++i)
        {
            if (strcmp(symbol_table[i].name, name) == 0 &&
                strcmp(symbol_table[i].scope, parent_scope) == 0)
                return symbol_table[i].type;
        }
    }

    // 3. Fallback to global (if needed)
    for (i = 0; i < symbol_count; ++i)
    {
        if (strcmp(symbol_table[i].name, name) == 0 &&
            strcmp(symbol_table[i].scope, "global") == 0)
            return symbol_table[i].type;
    }

    return NULL;
}
void panic_mode(const char *context);
void syntax_error(const char *message)
{
    FILE *error_log = fopen("error_log.txt", "a");
    if (error_log != NULL)
    {
        fprintf(error_log, "Syntax Error: %s at token %d (%s, \"%s\")\n",
                message, current_token,
                tokens[current_token].type,
                tokens[current_token].lexeme);
        fclose(error_log);
    }

    printf("Syntax Error: %s at token %d (%s, \"%s\")\n",
           message, current_token,
           tokens[current_token].type,
           tokens[current_token].lexeme);

    panic_mode("syntax_error");
}

#define MAX_IR 1024
#define MAX_SCOPE_DEPTH 16

typedef struct
{
    char op[16];
    char arg1[64];
    char arg2[64];
    char result[64];
} IR;

IR ir_list[MAX_IR];
int ir_count = 0;
int temp_count = 0;
int label_count = 0;
char current_temp[MAX_LEXEME_LEN];
char last_temp[MAX_LEXEME_LEN];

char *new_temp()
{
    static char buffer[64];
    sprintf(buffer, "t%d", temp_count++);
    insert_symbol(buffer, current_decl_type, current_scope); // Or infer type from context
    return strdup(buffer);
}

char *new_label()
{
    static char buffer[64];
    sprintf(buffer, "L%d", label_count++);
    return strdup(buffer);
}

void emit(const char *op, const char *arg1, const char *arg2, const char *result)
{
    if (ir_count >= MAX_IR)
    {
        fprintf(stderr, "IR list overflow\n");
        exit(1);
    }

    // Handle assignment case where only arg1 is provided (e.g., current_temp = func_or_var_name)
    if (op != NULL && arg1 != NULL && arg2 == NULL)
    {
        strcpy(ir_list[ir_count].op, "=");        // Use "=" for assignment
        strcpy(ir_list[ir_count].arg1, arg1);     // First operand (func_or_var_name)
        strcpy(ir_list[ir_count].arg2, "");       // No second operand
        strcpy(ir_list[ir_count].result, result); // Result (current_temp)
    }
    // Handle comparison operators (e.g., ==, <, >)
    else if (op != NULL && (strcmp(op, "==") == 0 || strcmp(op, "<") == 0 || strcmp(op, ">") == 0 || strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0))
    {
        // For comparisons, the result of the comparison is stored in a temporary variable (result)
        strcpy(ir_list[ir_count].op, op);         // Comparison operator (==, <, >, etc.)
        strcpy(ir_list[ir_count].arg1, arg1);     // First operand (e.g., a)
        strcpy(ir_list[ir_count].arg2, arg2);     // Second operand (e.g., b)
        strcpy(ir_list[ir_count].result, result); // Temporary variable to store the result of the comparison (e.g., t1)
    }
    // Handle general binary operations (e.g., addition, subtraction)
    else
    {
        strcpy(ir_list[ir_count].op, op);         // Binary operator (e.g., +, -, *)
        strcpy(ir_list[ir_count].arg1, arg1);     // First operand
        strcpy(ir_list[ir_count].arg2, arg2);     // Second operand
        strcpy(ir_list[ir_count].result, result); // Result (temporary variable)
    }

    ir_count++;
}

void print_ir()
{
    int i;
    printf("\n=== INTERMEDIATE CODE (IR) ===\n");
    for (i = 0; i < ir_count; ++i)
    {
        if (strcmp(ir_list[i].op, "label") == 0)
        {
            printf("%s:\n", ir_list[i].result);
        }
        else if (strcmp(ir_list[i].op, "ifgoto") == 0)
        {
            printf("if %s goto %s\n", ir_list[i].arg1, ir_list[i].result);
        }
        else if (strcmp(ir_list[i].op, "goto") == 0)
        {
            printf("goto %s\n", ir_list[i].result);
        }
        else if (strcmp(ir_list[i].op, "param") == 0)
        {
            printf("param %s\n", ir_list[i].arg1);
        }
        else if (strcmp(ir_list[i].op, "call") == 0)
        {
            printf("%s = call %s\n", ir_list[i].result, ir_list[i].arg1);
        }
        else if (strcmp(ir_list[i].op, "=") == 0 && strcmp(ir_list[i].arg2, "") == 0)
        {
            printf("%s = %s\n", ir_list[i].result, ir_list[i].arg1);
        }
        else
        {
            printf("%s = %s %s %s\n", ir_list[i].result, ir_list[i].arg1, ir_list[i].op, ir_list[i].arg2);
        }
    }
    printf("==============================\n\n");
}

// ==== Parser declarations ====

void parse_program();
void parse_fdecls();
void parse_fdecls_prime();
void parse_fdec();
void parse_params();
void parse_params_prime();
void parse_declarations();
void parse_declarations_prime();
void parse_decl();
void parse_type();
void parse_varlist();
void parse_varlist_prime();
void parse_statement_seq();
void parse_statement_seq_prime();
void parse_statement();
void parse_statement_prime();
void parse_expr();
void parse_expr_prime();
void parse_term();
void parse_term_prime();
void parse_factor();
void parse_var();
void parse_var_prime();
void parse_id();
void parse_bexpr();
void parse_bexpr_prime();
void parse_bterm();
void parse_bterm_prime();
void parse_bfactor();
char *parse_comp();
void parse_exprseq();
void parse_exprseq_prime();

int is_sync_token();

// ==== Main entry ====

int main()
{
    // puts(">>> Main begins");
    // printf("Starting token load...\n"); // Debug message
    load_tokens("C:\\cp471\\phase1\\Lexeme.txt");
    // printf("Token load completed\n"); // Debug message
    FILE *error_log = fopen("error_log.txt", "w");
    parse_program();

    Token t = peek_token();
    if (strcmp(t.type, "Start") == 0 && strcmp(t.lexeme, ".") == 0)
    {
        printf("Parsing completed successfully.\n");
    }
    else
    {
        syntax_error("Expected end of input ('.')");
        return;
    }
    print_symbol_table(); // show table after successful parse
    print_ir();
    return 0;
}

// ==== Recursive Descent Parser ====

void parse_program()
{
    Token t = peek_token();

    while (!(strcmp(t.type, "Start") == 0 && strcmp(t.lexeme, ".") == 0))
    {
        if (strcmp(t.lexeme, "def") == 0)
        {
            parse_fdec();
        }
        else
        {
            parse_statement_seq();
        }

        t = peek_token(); // update token after parsing
    }
    return;
}

void parse_fdecls()
{
    Token t = peek_token();
    if (strcmp(t.lexeme, "def") == 0)
    {
        parse_fdec();
        // if (!match_token("DELIM", ";"))
        //  syntax_error("Expected ';' after function declaration");
        parse_fdecls_prime();
    }
}

void parse_fdecls_prime()
{
    Token t = peek_token();
    if (strcmp(t.lexeme, "def") == 0)
    {
        parse_fdec();

        parse_fdecls_prime();
    }
}

void parse_fdec()
{
    if (!match_token("KEYWORD", "def"))
        syntax_error("Expected 'def'");
    return;
    parse_type();
    parse_id();
    strcpy(current_scope, tokens[current_token - 1].lexeme);     // function name
    strcpy(current_decl_type, tokens[current_token - 2].lexeme); // return type
    insert_symbol(current_scope, current_decl_type, "global");   // global function entry

    if (!match_token("DELIM", "("))
        syntax_error("Expected '(' after function name");
    return;
    parse_params();
    if (!match_token("DELIM", ")"))
        syntax_error("Expected ')' after parameters");
    return;
    parse_declarations();
    parse_statement_seq();
    // printf("Looking for 'fed'... current: %s,%s\n", tokens[current_token].type, tokens[current_token].lexeme);

    if (!match_token("KEYWORD", "fed"))
        syntax_error("Expected 'fed' to end function");
    return;

    // printf("Looking for ';' after 'fed'... current: %s,%s\n", tokens[current_token].type, tokens[current_token].lexeme);

    if (!match_token("DELIM", ";"))
        syntax_error("Expected ';' after 'fed'");
    return;
    return;
}

void parse_params()
{
    Token t = peek_token();
    if (strcmp(t.lexeme, "int") == 0 || strcmp(t.lexeme, "double") == 0)
    {
        parse_type();
        Token id = peek_token();
        parse_id();
        insert_symbol(id.lexeme, current_decl_type, current_scope);

        parse_params_prime();
    }
}

void parse_params_prime()
{
    if (match_token("DELIM", ","))
    {
        parse_params();
    }
}

void parse_declarations()
{
    Token t = peek_token();
    if (strcmp(t.lexeme, "int") == 0 || strcmp(t.lexeme, "double") == 0)
    {
        parse_decl();
        if (!match_token("DELIM", ";"))
            syntax_error("Expected ';' after declaration");
        return;
        parse_declarations_prime();
    }
}

void parse_declarations_prime()
{
    Token t = peek_token();
    if (strcmp(t.lexeme, "int") == 0 || strcmp(t.lexeme, "double") == 0)
    {
        parse_decl();
        // if (!match_token("DELIM", ";"))
        // syntax_error("Expected ';' after declaration");
        parse_declarations_prime();
    }
}

void parse_decl()
{
    parse_type();
    parse_varlist();
}

void parse_type()
{
    if (match_token("KEYWORD", "int"))
        strcpy(current_decl_type, "int");
    else if (match_token("KEYWORD", "double"))
        strcpy(current_decl_type, "double");
    else
        syntax_error("Expected type 'int' or 'double'");
    return;
}

void parse_varlist()
{
    parse_id();
    const char *var_name = tokens[current_token - 1].lexeme;
    insert_symbol(var_name, current_decl_type, current_scope);

    // Check for initializer
    if (match_token("OP", "="))
    {
        parse_expr(); // Fills current_expr_type and current_temp

        // Type check
        if (strcmp(current_decl_type, current_expr_type) != 0)
        {
            fprintf(stderr, "Type Error: Cannot initialize variable '%s' of type '%s' with type '%s'\n",
                    var_name, current_decl_type, current_expr_type);
            exit(1);
        }

        emit("=", current_temp, NULL, var_name); // e.g., x = t0
    }

    parse_varlist_prime(); // Handle comma-separated vars
}

void parse_varlist_prime()
{
    if (match_token("DELIM", ","))
    {
        parse_varlist();
    }
}

void parse_statement_seq()
{
    Token t = peek_token();

    // Allow local declarations inside blocks
    while (strcmp(t.lexeme, "int") == 0 || strcmp(t.lexeme, "double") == 0)
    {
        parse_declarations();
        t = peek_token(); // update after declaration
    }

    if ((strcmp(t.type, "KEYWORD") == 0 && strcmp(t.lexeme, "fed") == 0) ||
        (strcmp(t.type, "KEYWORD") == 0 && strcmp(t.lexeme, "fi") == 0) ||
        (strcmp(t.type, "KEYWORD") == 0 && strcmp(t.lexeme, "od") == 0))
        return;

    parse_statement();
    parse_statement_seq_prime();
}

void parse_statement_seq_prime()
{
    if (match_token("DELIM", ";"))
    {
        // Stop if next token is 'fed' or '.'
        if ((strcmp(tokens[current_token].type, "KEYWORD") == 0 &&
             strcmp(tokens[current_token].lexeme, "fed") == 0))
        {
            return;
        }

        // if ((strcmp(tokens[current_token].type, "Start") == 0 &&
        //  strcmp(tokens[current_token].lexeme, ".") == 0))
        //{
        //    return;
        //}
        parse_statement();
        parse_statement_seq_prime();
    }
}
void parse_statement()
{
    Token t = peek_token();
    // printf("DEBUG statement: %s,%s\n", t.type, t.lexeme);
    if (strcmp(t.lexeme, "if") == 0)
    {
        match_token("KEYWORD", "if");
        parse_bexpr();

        if (!match_token("KEYWORD", "then"))
            syntax_error("Expected 'then'");
        return;

        char prev_scope[MAX_NAME_LEN];
        strcpy(prev_scope, current_scope);
        strcat(current_scope, "_if");

        sprintf(current_scope, "%s_if_%d", prev_scope, current_token);
        printf("[SCOPE ENTER] if-block -> %s\n", current_scope);

        parse_statement_seq();

        printf("[SCOPE EXIT] if-block -> %s\n", current_scope);
        strcpy(current_scope, prev_scope);

        parse_statement_prime();
    }
    else if (strcmp(t.lexeme, "while") == 0)
    {
        match_token("KEYWORD", "while");
        char *start_label = new_label();
        char *end_label = new_label();
        emit("label", NULL, NULL, start_label);
        parse_bexpr();
        emit("ifgoto", current_temp, NULL, end_label);

        if (!match_token("KEYWORD", "do"))
            syntax_error("Expected 'do'");
        return;

        char prev_scope[MAX_NAME_LEN];
        strcpy(prev_scope, current_scope);
        strcat(current_scope, "_while");

        sprintf(current_scope, "%s_while_%d", prev_scope, current_token);
        printf("[SCOPE ENTER] while-block -> %s\n", current_scope);

        parse_statement_seq();

        emit("goto", NULL, NULL, start_label);
        emit("label", NULL, NULL, end_label);

        printf("[SCOPE EXIT] while-block -> %s\n", current_scope);
        strcpy(current_scope, prev_scope); // restore scope

        if (!match_token("KEYWORD", "od"))
            syntax_error("Expected 'od'");
        return;
    }
    else if (strcmp(t.lexeme, "print") == 0)
    {
        match_token("KEYWORD", "print");
        // printf("DEBUG: match print\n");
        parse_expr();
        // printf("DEBUG after expr, current: %s (%s)\n", t.lexeme, t.type);
        // if (!match_token("DELIM", ";"))
        {
            // syntax_error("Expected ';' after print");
        }
    }
    else if (strcmp(t.lexeme, "return") == 0)
    {
        match_token("KEYWORD", "return");

        parse_expr();
        if (strcmp(current_expr_type, current_decl_type) != 0)
        {
            fprintf(stderr, "Type Error: Return type '%s' does not match function's declared return type '%s'\n",
                    current_expr_type, current_decl_type);
            exit(1);
        }
    }
    else if (strcmp(t.type, "ALPHA") == 0)
    {
        char varname[MAX_NAME_LEN];
        strcpy(varname, t.lexeme); // Save variable name
        parse_var();
        if (!match_token("OP", "="))
            syntax_error("Expected '='");
        return;

        const char *lhs_type = lookup_symbol_type(varname, current_scope);
        if (!lhs_type)
        {
            fprintf(stderr, "Semantic Error: Undeclared variable '%s'\n", varname);
            exit(1);
        }

        parse_expr();
        if (strcmp(lhs_type, current_expr_type) != 0)
        {
            fprintf(stderr, "Type Error: Cannot assign '%s' to variable '%s' of type '%s'\n",
                    current_expr_type, varname, lhs_type);
            exit(1);
        }
    }
    else
    {
        syntax_error("Invalid statement");
        return;
    }
}

void parse_statement_prime()
{
    if (match_token("KEYWORD", "else"))
    {
        parse_statement_seq();
    }
    if (!match_token("KEYWORD", "fi"))
        syntax_error("Expected 'fi'");
    return;
}

void parse_expr()
{
    parse_term();
    strcpy(last_temp, current_temp); // save temp for chaining
    parse_expr_prime();
}

void parse_expr_prime()
{
    Token t = peek_token();
    if (strcmp(t.lexeme, "+") == 0 || strcmp(t.lexeme, "-") == 0)
    {
        char op[4];
        strcpy(op, t.lexeme);
        char lhs_type[MAX_TYPE_LEN];
        char lhs_temp[MAX_LEXEME_LEN];

        strcpy(lhs_type, current_expr_type);
        strcpy(lhs_temp, last_temp);

        next_token(); // consume + or -

        parse_term();

        // type checking
        if (strcmp(lhs_type, current_expr_type) != 0)
        {
            fprintf(stderr, "Type Error: Mismatched types in expression: '%s' and '%s'\n",
                    lhs_type, current_expr_type);
            exit(1);
        }

        char *result_temp = new_temp();
        emit(op, lhs_temp, current_temp, result_temp);
        strcpy(current_temp, result_temp);
        strcpy(last_temp, result_temp);

        parse_expr_prime();
    }
}

void parse_term()
{
    parse_factor();
    strcpy(last_temp, current_temp); // save temp for chaining
    parse_term_prime();
}

void parse_term_prime()
{
    Token t = peek_token();
    if (strcmp(t.lexeme, "*") == 0 || strcmp(t.lexeme, "/") == 0 || strcmp(t.lexeme, "%") == 0)
    {
        char op[4];
        strcpy(op, t.lexeme);
        char lhs_type[MAX_TYPE_LEN];
        char lhs_temp[MAX_LEXEME_LEN];

        strcpy(lhs_type, current_expr_type);
        strcpy(lhs_temp, last_temp);

        next_token(); // consume op
        parse_factor();

        if (strcmp(lhs_type, current_expr_type) != 0)
        {
            fprintf(stderr, "Type Error: Mismatched types in term: '%s' and '%s'\n",
                    lhs_type, current_expr_type);
            exit(1);
        }

        char *result_temp = new_temp();
        emit(op, lhs_temp, current_temp, result_temp);
        strcpy(current_temp, result_temp);
        strcpy(last_temp, result_temp);

        parse_term_prime();
    }
}

void parse_factor()
{
    Token t = peek_token();

    if (strcmp(t.lexeme, "(") == 0)
    {
        match_token("DELIM", "(");
        parse_expr();
        if (!match_token("DELIM", ")"))
            syntax_error("Expected ')' after expression");
        return;
    }
    else if (strcmp(t.type, "NUM") == 0 || strcmp(t.type, "DOUBLE") == 0)
    {
        next_token(); // consume number
        strcpy(current_expr_type, strcmp(t.type, "NUM") == 0 ? "int" : "double");
        strcpy(current_temp, new_temp());
        emit("=", t.lexeme, NULL, current_temp);
    }
    else if (strcmp(t.type, "ALPHA") == 0)
    {
        const char *type = lookup_symbol_type(t.lexeme, current_scope);
        if (!type)
            type = lookup_symbol_type(t.lexeme, "global");
        if (!type)
        {
            fprintf(stderr, "Semantic Error: Undeclared variable '%s'\n", t.lexeme);
            exit(1);
        }
        strcpy(current_expr_type, type);

        char func_or_var_name[MAX_LEXEME_LEN];
        strcpy(func_or_var_name, t.lexeme); // store before advancing

        parse_id(); // consume ID
        t = peek_token();

        if (strcmp(t.lexeme, "(") == 0)
        {
            match_token("DELIM", "(");
            parse_exprseq(); // will emit param instructions
            if (!match_token("DELIM", ")"))
                syntax_error("Expected ')' after function call");
            return;

            char *func_temp = new_temp();
            emit("call", func_or_var_name, NULL, func_temp); // <- correct usage
            strcpy(current_temp, func_temp);
        }
        else
        {
            parse_var_prime();
            strcpy(current_temp, new_temp());
            emit("=", func_or_var_name, NULL, current_temp);
            // treat as variable usage
        }
    }
    else
    {
        syntax_error("Invalid factor");
        return;
    }
}

void parse_var()
{
    parse_id();
    parse_var_prime();
}

void parse_var_prime()
{
    if (match_token("DELIM", "["))
    {
        parse_expr();
        if (!match_token("DELIM", "]"))
            syntax_error("Expected ']'");
        return;
    }
}

void parse_id()
{
    Token t = peek_token();
    // printf("DEBUG parse_id: %s,%s\n", t.type, t.lexeme);
    if (strcmp(t.type, "ALPHA") == 0)
        next_token();
    else
        syntax_error("Expected identifier");
    return;
}

void parse_bexpr()
{
    parse_bterm();       // Parse the first boolean term
    parse_bexpr_prime(); // Check for further boolean expression (logical operators like 'or')
    // printf("DEBUG: After parsing bexpr\n");
}

void parse_bexpr_prime()
{
    Token t = peek_token();
    if (strcmp(t.lexeme, "or") == 0) // If there's a logical OR operator
    {
        match_token("KEYWORD", "or"); // Match the 'or' keyword
        parse_bterm();                // Parse the next boolean term
        parse_bexpr_prime();          // Recursively check for further OR operators
    }
}

void parse_bterm()
{
    parse_bfactor();     // Parse the first factor (which could be a comparison or a variable)
    parse_bterm_prime(); // Check for further boolean terms (e.g., 'and' operators)
}

void parse_bterm_prime()
{
    Token t = peek_token();
    if (strcmp(t.lexeme, "and") == 0) // If there's a logical AND operator
    {
        match_token("KEYWORD", "and"); // Match the 'and' keyword
        parse_bfactor();               // Parse the next boolean factor
        parse_bterm_prime();           // Recursively check for further AND operators
    }
}

void parse_bfactor()
{
    Token t = peek_token();
    if (strcmp(t.lexeme, "(") == 0)
    {
        next_token(); // consume '('

        // Parse left-hand side expression
        parse_expr();
        char lhs_temp[MAX_LEXEME_LEN];
        strcpy(lhs_temp, current_temp);

        // Parse comparison operator
        char *comp_op = parse_comp();

        // Parse right-hand side expression
        parse_expr();
        char rhs_temp[MAX_LEXEME_LEN];
        strcpy(rhs_temp, current_temp);

        // Check for closing ')'
        t = peek_token();
        if (strcmp(t.lexeme, ")") != 0)
        {
            syntax_error("Expected ')' after comparison");
            return;
        }
        next_token(); // consume ')'

        // Emit the comparison
        char *result_temp = new_temp();
        emit(comp_op, lhs_temp, rhs_temp, result_temp);
        strcpy(current_temp, result_temp); // store for future chaining
    }
    else
    {
        syntax_error("Expected '(' in boolean factor");
        return;
    }
}

char *parse_comp()
{
    Token t = peek_token();

    if (strcmp(t.lexeme, "<") == 0 || strcmp(t.lexeme, ">") == 0 ||
        strcmp(t.lexeme, "==") == 0 || strcmp(t.lexeme, "<=") == 0 ||
        strcmp(t.lexeme, ">=") == 0 || strcmp(t.lexeme, "<>") == 0)
    {
        next_token();            // Consume the operator
        return strdup(t.lexeme); // Return a copy of the operator
    }
    else
    {
        syntax_error("Expected comparison operator");
        return NULL;
    }
}
void parse_exprseq()
{
    Token t = peek_token();

    // Check if the argument list is non-empty
    if (strcmp(t.type, "NUM") == 0 || strcmp(t.type, "ALPHA") == 0 || strcmp(t.lexeme, "(") == 0 || strcmp(t.type, "DOUBLE") == 0)
    {
        parse_expr();
        emit("=", t.lexeme, NULL, current_temp);

        parse_exprseq_prime(); // Parse the rest (if any)
    }
    else
    {
        // epsilon production: empty argument list (e.g., f())
        // printf("DEBUG: Empty argument list (epsilon in exprseq)\n");
        return;
    }
}

void parse_exprseq_prime()
{
    if (match_token("DELIM", ","))
    {
        // printf("DEBUG: Comma found, parsing next argument in exprseq\n");
        parse_expr();          // Parse the next argument
        parse_exprseq_prime(); // Continue checking for more arguments
    }
    else
    {
        // epsilon production: no more arguments
        // printf("DEBUG: No more arguments (epsilon in exprseq_prime)\n");
    }
}
void panic_mode(const char *context)
{
    FILE *error_log = fopen("error_log.txt", "a");
    if (error_log != NULL)
    {
        fprintf(error_log, "Entering panic mode recovery in context: %s\n", context);
        fclose(error_log);
    }

    /* Skip tokens until sync token or '.' or EOF */
    while (current_token < token_count &&
           tokens[current_token].type != NULL &&
           !is_sync_token(tokens[current_token].type) &&
           (tokens[current_token].lexeme == NULL || strcmp(tokens[current_token].lexeme, ".") != 0))
    {
        current_token++;
    }

    /* Move to next token if not EOF or final '.' */
    if (current_token < token_count &&
        tokens[current_token].lexeme != NULL &&
        strcmp(tokens[current_token].lexeme, ".") != 0)
    {
        next_token();
    }
}

int is_sync_token(const char *Lexeme)
{
    return strcmp(Lexeme, ";") == 0 ||     /* ; */
           strcmp(Lexeme, "fi") == 0 ||    /* fi */
           strcmp(Lexeme, "else") == 0 ||  /* else */
           strcmp(Lexeme, "fed") == 0 ||   /* fed */
           strcmp(Lexeme, "then") == 0 ||  /* then */
           strcmp(Lexeme, "print") == 0 || /* print */
           strcmp(Lexeme, "if") == 0 ||    /* if */
           strcmp(Lexeme, "while") == 0;   /* while */
}
