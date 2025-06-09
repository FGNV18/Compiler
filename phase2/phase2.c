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

void syntax_error(const char *message)
{
    FILE *error_log;

    /* Open the file for writing ("w" mode overwrites if file exists) */
    error_log = fopen("error_log.txt", "a");

    /* Check if the file opened successfully */

    fprintf(error_log, "Syntax Error: %s at token %d (%s,%s)\n", message, current_token,
            tokens[current_token].type, tokens[current_token].lexeme);

    next_token();

    fclose(error_log);
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
void parse_comp();
void parse_exprseq();
void parse_exprseq_prime();

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
    }
    print_symbol_table(); // show table after successful parse
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
    parse_type();
    parse_id();
    strcpy(current_scope, tokens[current_token - 1].lexeme);     // function name
    strcpy(current_decl_type, tokens[current_token - 2].lexeme); // return type
    insert_symbol(current_scope, current_decl_type, "global");   // global function entry

    if (!match_token("DELIM", "("))
        syntax_error("Expected '(' after function name");
    parse_params();
    if (!match_token("DELIM", ")"))
        syntax_error("Expected ')' after parameters");
    parse_declarations();
    parse_statement_seq();
    // printf("Looking for 'fed'... current: %s,%s\n", tokens[current_token].type, tokens[current_token].lexeme);

    if (!match_token("KEYWORD", "fed"))
        syntax_error("Expected 'fed' to end function");

    // printf("Looking for ';' after 'fed'... current: %s,%s\n", tokens[current_token].type, tokens[current_token].lexeme);

    if (!match_token("DELIM", ";"))
        syntax_error("Expected ';' after 'fed'");
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
}

void parse_varlist()
{
    parse_id();
    insert_symbol(tokens[current_token - 1].lexeme, current_decl_type, current_scope);
    parse_varlist_prime();
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
        parse_bexpr();
        if (!match_token("KEYWORD", "do"))
            syntax_error("Expected 'do'");

        char prev_scope[MAX_NAME_LEN];
        strcpy(prev_scope, current_scope);
        strcat(current_scope, "_while");

        sprintf(current_scope, "%s_while_%d", prev_scope, current_token);
        printf("[SCOPE ENTER] while-block -> %s\n", current_scope);

        parse_statement_seq();

        printf("[SCOPE EXIT] while-block -> %s\n", current_scope);
        strcpy(current_scope, prev_scope); // restore scope

        if (!match_token("KEYWORD", "od"))
            syntax_error("Expected 'od'");
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

        const char *expected = lookup_symbol_type(current_scope, "global");
        if (!expected)
            expected = "int";

        parse_expr();
    }
    else if (strcmp(t.type, "ALPHA") == 0)
    {
        char varname[MAX_NAME_LEN];
        strcpy(varname, t.lexeme); // Save variable name
        parse_var();
        if (!match_token("OP", "="))
            syntax_error("Expected '='");

        const char *lhs_type = lookup_symbol_type(varname, current_scope);
        if (!lhs_type)
        {
            fprintf(stderr, "Semantic Error: Undeclared variable in statement '%s'\n", varname);
            exit(1);
        }
        char expected_type[MAX_TYPE_LEN];
        strcpy(expected_type, lhs_type);

        parse_expr();
    }
    else
    {
        syntax_error("Invalid statement");
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
}

void parse_expr()
{
    Token t = peek_token();
    // printf(">> ENTER parse_expr with token: %s (%s)\n", t.lexeme, t.type);
    parse_term();
    parse_expr_prime();
    t = peek_token();
    // printf("<< EXIT  parse_expr with token: %s (%s)\n", t.lexeme, t.type);
    return;
}

void parse_expr_prime()
{
    Token t = peek_token();

    if (strcmp(t.lexeme, "+") == 0 || strcmp(t.lexeme, "-") == 0)
    {
        // printf("DEBUG expr_prime operator: %s\n", t.lexeme);
        next_token();
        parse_term();
        parse_expr_prime();
    }
    else
    {
        // printf("DEBUG expr_prime: returning epsilon on %s,%s\n", t.type, t.lexeme);
        //   only log, do nothing – epsilon production
    }
}

void parse_term()
{
    // printf("DEBUG term entry\n");
    parse_factor();
    // printf("DEBUG after factor\n");
    parse_term_prime();

    // printf("DEBUG term exit\n");
    return;
}

void parse_term_prime()
{
    Token t = peek_token();
    if (strcmp(t.lexeme, "*") == 0 || strcmp(t.lexeme, "/") == 0 || strcmp(t.lexeme, "%") == 0)
    {
        next_token();
        parse_factor();
        parse_term_prime();
    }
    else
    {
        // EPSILON: do nothing
        // printf("DEBUG term_prime: epsilon\n");
        return;
    }
}

void parse_factor()
{
    // printf("DEBUG: enter factor\n");
    if (current_token >= token_count)
    {
        printf("ERROR: Reached end of token stream in parse_factor()\n");
        exit(1);
    }

    Token t = peek_token();
    // printf("DEBUG factor: %s,%s\n", t.type, t.lexeme);

    if (strcmp(t.lexeme, "(") == 0)
    {
        match_token("DELIM", "(");
        parse_expr();
        if (!match_token("DELIM", ")"))
            syntax_error("Expected ')' after expression");
    }
    else if (strcmp(t.type, "NUM") == 0)
    {
        next_token(); // consume the number
        strcpy(current_expr_type, "int");
    }
    else if (strcmp(t.type, "DOUBLE") == 0)
    {
        next_token(); // consume the number
        strcpy(current_expr_type, "double");
    }
    else if (strcmp(t.type, "ALPHA") == 0)
    {
        const char *type = lookup_symbol_type(t.lexeme, current_scope);
        if (!type)
            type = lookup_symbol_type(t.lexeme, "global");
        if (!type)
        {
            fprintf(stderr, "Semantic Error: Undeclared variable in factor '%s'\n", t.lexeme);
            exit(1);
        }

        strcpy(current_expr_type, type); // e.g., "int" or "double"

        parse_id(); // Always consume the ID

        t = peek_token(); // lookahead for function call or variable
        if (strcmp(t.lexeme, "(") == 0)
        {
            // printf("DEBUG: Detected function call after identifier\n");
            match_token("DELIM", "(");
            parse_exprseq();
            if (!match_token("DELIM", ")"))
                syntax_error("Expected ')' after function arguments");
        }
        else
        {
            parse_var_prime();
        }
    }
    else
    {
        syntax_error("Invalid factor");
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
        // printf("DEBUG: In parse_bfactor, Token: %s,%s\n", t.type, t.lexeme);
        next_token(); // consume '('

        t = peek_token();
        // printf("DEBUG: After '(', got token: %s,%s\n", t.type, t.lexeme);

        // printf("DEBUG: Calling parse_expr()\n");
        parse_expr(); // Parse the first expression
        // puts("!!! We returned from parse_expr() and are still in bfactor");

        // **Ensure that parse_comp() is called after the first expression**
        // Debugging: after parsing expr, we want to make sure we’re getting a comparison operator
        // printf("DEBUG parse_id: %s,%s\n", t.type, t.lexeme);
        // puts("!!! About to call parse_comp()");
        parse_comp(); // Now check for and consume comparison operator

        // printf("DEBUG: Calling parse_expr() again\n");
        parse_expr(); // Parse the second expression

        t = peek_token();
        if (strcmp(t.lexeme, ")") == 0)
        {
            next_token(); // consume ')'
        }
        else
        {
            syntax_error("Expected ')'");
        }
    }
    else
    {
        syntax_error("Expected '(' in bfactor");
    }
}

void parse_comp()
{

    // printf("DEBUG: Entered parse_comp\n");
    Token t = peek_token();
    if (strcmp(t.lexeme, "<") == 0 || strcmp(t.lexeme, ">") == 0 ||
        strcmp(t.lexeme, "==") == 0 || strcmp(t.lexeme, "<=") == 0 ||
        strcmp(t.lexeme, ">=") == 0 || strcmp(t.lexeme, "<>") == 0) // Handle comparison operators
    {
        next_token(); // Consume the comparison operator
    }
    else
    {
        syntax_error("Expected comparison operator");
    }
}
void parse_exprseq()
{
    Token t = peek_token();

    // Check if the argument list is non-empty
    if (strcmp(t.type, "NUM") == 0 || strcmp(t.type, "ALPHA") == 0 || strcmp(t.lexeme, "(") == 0 || strcmp(t.type, "DOUBLE") == 0)
    {
        parse_expr();          // Parse the first argument
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
