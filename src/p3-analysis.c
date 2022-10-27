/**
 * @file p3-analysis.c
 * @brief Compiler phase 3: static analysis
 * @name Vivian Stewart and Katie Brasacchio
 */
#include "p3-analysis.h"

/**
 * @brief State/data for static analysis visitor
 */
typedef struct AnalysisData
{
    /**
     * @brief List of errors detected
     */
    ErrorList *errors;
    bool is_loop;
    bool is_return;
    bool is_conditional;
    DecafType funcdecl_return_type;

    /* BOILERPLATE: TODO: add any new desired state information (and clean it up in AnalysisData_free) */

} AnalysisData;

/**
 * @brief Allocate memory for analysis data
 *
 * @returns Pointer to allocated structure
 */
AnalysisData *AnalysisData_new()
{
    AnalysisData *data = (AnalysisData *)calloc(1, sizeof(AnalysisData));
    CHECK_MALLOC_PTR(data);
    data->errors = ErrorList_new();
    return data;
}

/**
 * @brief Deallocate memory for analysis data
 *
 * @param data Pointer to the structure to be deallocated
 */
void AnalysisData_free(AnalysisData *data)
{
    /* free everything in data that is allocated on the heap except the error
     * list; it needs to be returned after the analysis is complete */

    /* free "data" itself */
    free(data);
}

/**
 * @brief Macro for more convenient access to the data inside a @ref AnalysisVisitor
 * data structure
 */
#define DATA ((AnalysisData *)visitor->data)

/**
 * @brief Macro for more convenient access to the error list inside a
 * @ref AnalysisVisitor data structure
 */
#define ERROR_LIST (((AnalysisData *)visitor->data)->errors)

/**
 * @brief Wrapper for @ref lookup_symbol that reports an error if the symbol isn't found
 *
 * @param visitor Visitor with the error list for reporting
 * @param node AST node to begin the search at
 * @param name Name of symbol to find
 * @returns The @ref Symbol if found, otherwise @c NULL
 */
Symbol *lookup_symbol_with_reporting(NodeVisitor *visitor, ASTNode *node, const char *name)
{
    Symbol *symbol = lookup_symbol(node, name);

    if (symbol == NULL)
    {
        ErrorList_printf(ERROR_LIST, "Symbol '%s' undefined on line %d", name, node->source_line);
    }
    return symbol;
}

/**
 * @brief Macro for shorter storing of the inferred @c type attribute
 */
#define SET_INFERRED_TYPE(T) ASTNode_set_printable_attribute(node, "type", (void *)(T), \
                                                             type_attr_print, dummy_free)

/**
 * @brief Macro for shorter retrieval of the inferred @c type attribute
 */
#define GET_INFERRED_TYPE(N) (DecafType) ASTNode_get_attribute(N, "type")

void find_ducplicat_helper(ASTNode *node)
{
    SymbolTable *table = (SymbolTable *)ASTNode_get_attribute(node, "symbolTable");
    int count = 0;
    FOR_EACH(Symbol *, s1, table->local_symbols)
    {
        count = 0;
        FOR_EACH(Symbol *, s2, table->local_symbols)
        {
            if (strcmp(s1->name, s2->name) == 0)
            {
                count = count + 1;

                if (count > 1)
                {
                    Error_throw_printf("Duplicate names %s\n", s1->name);
                }
            }
        }
    }
}

/**
 * @brief previsit program node
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_program(NodeVisitor *visitor, ASTNode *node)
{
    if (lookup_symbol(node, "main") == NULL || lookup_symbol(node, "main")->symbol_type != FUNCTION_SYMBOL)
    {
        Error_throw_printf("Program does not contain a 'main' function\n");
    }
    find_ducplicat_helper(node);
}

/**
 * @brief postvisit program
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_program(NodeVisitor *visitor, ASTNode *node)
{
    if (lookup_symbol(node, "main")->type != INT)
    {
        Error_throw_printf("Program 'main' function must return an int\n");
    }
}

/**
 * @brief previsit vardecl
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_vardecl(NodeVisitor *visitor, ASTNode *node)
{
    SET_INFERRED_TYPE(node->vardecl.type);
}

/**
 * @brief postvisit vardecl
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_vardecl(NodeVisitor *visitor, ASTNode *node)
{
    DecafType type = GET_INFERRED_TYPE(node);
    if (type == VOID)
    {
        Error_throw_printf("Void variable '%s' on line %d", node->vardecl.name, node->source_line);
    }

    // check for valid array size in declaration
    if (node->vardecl.is_array)
    {
        if (node->vardecl.array_length <= 0)
        {
            Error_throw_printf("Array length must be greater than 0\n");
        }
    }
}

/**
 * @brief previsit funcdecl
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_funcdecl(NodeVisitor *visitor, ASTNode *node)
{
    SET_INFERRED_TYPE(node->funcdecl.return_type);
    // Error_throw_printf("\n  hi %s \n", DecafType_to_string(GET_INFERRED_TYPE(node)));
    DATA->funcdecl_return_type = node->funcdecl.return_type;
}

/**
 * @brief postvisit funcdecl
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_funcdecl(NodeVisitor *visitor, ASTNode *node)
{
    find_ducplicat_helper(node);
}

/**
 * @brief previsit block
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_block(NodeVisitor *visitor, ASTNode *node)
{
}

/**
 * @brief postvisit block
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_block(NodeVisitor *visitor, ASTNode *node)
{
    find_ducplicat_helper(node);
}

/**
 * @brief previsit assignment
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_assignment(NodeVisitor *visitor, ASTNode *node)
{
    // do nothing
}

/**
 * @brief postisit assignment
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_assignment(NodeVisitor *visitor, ASTNode *node)
{
    DecafType left_type = GET_INFERRED_TYPE(node->assignment.location);
    DecafType right_type = GET_INFERRED_TYPE(node->assignment.value);
    if (left_type != right_type)
    {
        Error_throw_printf("Expected %s type but type was %s\n", DecafType_to_string(left_type), DecafType_to_string(right_type));
    }
}

/**
 * @brief previsit conditional
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_conditional(NodeVisitor *visitor, ASTNode *node)
{
    DATA->is_conditional = true;
}

/**
 * @brief postvisit conditional
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_conditional(NodeVisitor *visitor, ASTNode *node)
{
    DATA->is_conditional = false;

    DecafType cond_type = GET_INFERRED_TYPE(node->conditional.condition);

    if (cond_type != BOOL)
    {
        Error_throw_printf("Conditional type was %s, expected bool on line %d\n", DecafType_to_string(cond_type), node->source_line);
    }
}

/**
 * @brief previsit while loop
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_while_loop(NodeVisitor *visitor, ASTNode *node)
{
    DATA->is_loop = true;
}

/**
 * @brief postvisit while loop
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_while_loop(NodeVisitor *visitor, ASTNode *node)
{
    DATA->is_loop = false;
    DecafType cond_type = GET_INFERRED_TYPE(node->whileloop.condition);

    if (cond_type != BOOL)
    {
        Error_throw_printf("Conditional type was %s, expected bool on line %d\n", DecafType_to_string(cond_type), node->source_line);
    }
}

/**
 * @brief previsit return
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_return(NodeVisitor *visitor, ASTNode *node)
{
    DATA->is_return = true;
}

/**
 * @brief postvisit return
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_return(NodeVisitor *visitor, ASTNode *node)
{
    DATA->is_return = false;
    DecafType infer_return_value = GET_INFERRED_TYPE(node->funcreturn.value);
    if (DATA->funcdecl_return_type != infer_return_value)
    {
        Error_throw_printf("Invalid return type, Expected %s was %s on line %d\n", DecafType_to_string(DATA->funcdecl_return_type), DecafType_to_string(infer_return_value), node->source_line);
    }
}

/**
 * @brief previsit break
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_break(NodeVisitor *visitor, ASTNode *node)
{
    if (!DATA->is_loop)
    {
        Error_throw_printf("Break statement should be inside a while loop.\n");
    }
}

/**
 * @brief postvisit break
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_break(NodeVisitor *visitor, ASTNode *node)
{
}

/**
 * @brief previsit continue
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_continue(NodeVisitor *visitor, ASTNode *node)
{
    if (!DATA->is_loop)
    {
        Error_throw_printf("Continue statement should be inside a while loop.");
    }
}

/**
 * @brief postvisit continue
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_continue(NodeVisitor *visitor, ASTNode *node)
{
}

/**
 * @brief previsit binop
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_binop(NodeVisitor *visitor, ASTNode *node)
{
    BinaryOpType binop_type = node->binaryop.operator;
    // for >, >=, <, <=, +, -, *, /, %
    if ((binop_type > 3) && (binop_type < 13))
    {
        // set inferred type
        if (binop_type > 7 && binop_type < 13)
        {
            SET_INFERRED_TYPE(INT);
        }
        else
        {
            SET_INFERRED_TYPE(BOOL);
        }
        // == and !=
    }
    else if ((binop_type == 2) || (binop_type == 3) || (binop_type == 0) || (binop_type == 1))
    {
        SET_INFERRED_TYPE(BOOL);
    }
}

/**
 * @brief invisit binop
 *
 * @param visitor
 * @param node
 */
void Analysis_invisit_binop(NodeVisitor *visitor, ASTNode *node)
{
}

/**
 * @brief postvisit binop
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_binop(NodeVisitor *visitor, ASTNode *node)
{
    if (DATA->is_conditional || DATA->is_loop)
    {
        if (GET_INFERRED_TYPE(node) != BOOL)
        {
            Error_throw_printf("Invalid condition, must be boolean expression on line %d\n", node->source_line);
        }
    }

    BinaryOpType binop_type = node->binaryop.operator;
    DecafType left_type = GET_INFERRED_TYPE(node->binaryop.left);
    DecafType right_type = GET_INFERRED_TYPE(node->binaryop.right);
    // for >, >=, <, <=, +, -, *, /, %
    if ((binop_type > 3) && (binop_type < 13))
    {
        if (left_type != INT || right_type != INT)
        {
            Error_throw_printf("Cannot use operator %s on type %s and %s on line %d\n", BinaryOpToString(binop_type), DecafType_to_string(left_type), DecafType_to_string(right_type), node->source_line);
        }

        // == and !=
    }
    else if ((binop_type == 2) || (binop_type == 3))
    {
        if (left_type != right_type)
        {
            Error_throw_printf("Cannot use operator %s on type %s and %s on line %d\n", BinaryOpToString(binop_type), DecafType_to_string(left_type), DecafType_to_string(right_type), node->source_line);
        }
    }
    // || and &&
    else if ((binop_type == 0) || (binop_type == 1))
    {
        if (left_type != BOOL || right_type != BOOL)
        {
            Error_throw_printf("Cannot use operator %s on type %s and %s on line %d\n", BinaryOpToString(binop_type), DecafType_to_string(left_type), DecafType_to_string(right_type), node->source_line);
        }
    }
}

/**
 * @brief previsit unary operator
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_unop(NodeVisitor *visitor, ASTNode *node)
{
    UnaryOpType unop = node->unaryop.operator;
    if (unop == 0)
    {
        SET_INFERRED_TYPE(INT);
    }
    else
    {
        SET_INFERRED_TYPE(BOOL);
    }
}

/**
 * @brief postvisit unary operator
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_unop(NodeVisitor *visitor, ASTNode *node)
{
    DecafType actual_type = GET_INFERRED_TYPE(node->unaryop.child);
    DecafType inferred_type = GET_INFERRED_TYPE(node);
    if (actual_type != inferred_type)
    {
        Error_throw_printf("Type mismatch expected %s was %s on line %d\n", DecafType_to_string(inferred_type), DecafType_to_string(actual_type), node->source_line);
    }
}

/**
 * @brief previsit location
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_location(NodeVisitor *visitor, ASTNode *node)
{
    // Error_throw_printf("%s\n", lookup_symbol(node, node->location.name)->name);
    if (lookup_symbol_with_reporting(visitor, node, node->location.name) != NULL)
    {
        SET_INFERRED_TYPE(lookup_symbol_with_reporting(visitor, node, node->location.name)->type);
    }
    // else
    // {
    //     Error_throw_printf("here");
    // }
}

/**
 * @brief postvisit location
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_location(NodeVisitor *visitor, ASTNode *node)
{
    Symbol *sym = lookup_symbol_with_reporting(visitor, node, node->location.name);
    // Error_throw_printf("\n here \n");
    if (sym == NULL)
    {
        Error_throw_printf("Expected valid var on line %d\n", node->source_line);
    }

    // For array types
    if (sym != NULL && sym->symbol_type == 1 && node->location.index == NULL)
    {
        Error_throw_printf("Expected array index on line %d\n", node->source_line);
    }
    if (sym->symbol_type == 1 && GET_INFERRED_TYPE(node->location.index) != INT)
    {
        Error_throw_printf("Array index must be an integer on line %d\n", node->source_line);
    }
    // if (node->location.index->literal.integer < 0 || node->location.index->literal.integer >= sym->length)
    // {
    //     Error_throw_printf("Index out of bounds on line %d\n", node->source_line);
    // }
}

/**
 * @brief previsit function call
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_funcall(NodeVisitor *visitor, ASTNode *node)
{
    Symbol *sym = lookup_symbol_with_reporting(visitor, node, node->funccall.name);

    SET_INFERRED_TYPE(sym->type);
    // Error_throw_printf("\n here %s %s \n", DecafType_to_string(sym->type), node->funccall.name);
}

/**
 * @brief postvisit function call
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_funcall(NodeVisitor *visitor, ASTNode *node)
{
    Symbol *sym = lookup_symbol_with_reporting(visitor, node, node->funccall.name);

    if (sym->parameters->size != node->funccall.arguments->size)
    {
        Error_throw_printf("Incorrect number of arguments, expected %d, but got %d on line %d\n", sym->parameters->size, node->funccall.arguments->size, node->source_line);
    }
    for (int i = 0; i < sym->parameters->size; i++)
    {
        if (sym->parameters->head->type != GET_INFERRED_TYPE(node->funccall.arguments->head))
        {
            Error_throw_printf("Expected type %s but got type %s on line %d\n", DecafType_to_string(sym->parameters->head->type), DecafType_to_string(GET_INFERRED_TYPE(node->funccall.arguments->head)), node->source_line);
        }
        sym->parameters->head = sym->parameters->head->next;
        node->funccall.arguments->head = node->funccall.arguments->head->next;
    }
}

/**
 * @brief previsit literal
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_literal(NodeVisitor *visitor, ASTNode *node)
{
    SET_INFERRED_TYPE(node->literal.type);
}

/**
 * @brief postvisit literal
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_literal(NodeVisitor *visitor, ASTNode *node)
{
}

ErrorList *analyze(ASTNode *tree)
{
    // if (tree == NULL)
    // {
    //     Error_throw_printf("Tree is NULL\n");
    // }
    /* allocate analysis structures */
    NodeVisitor *v = NodeVisitor_new();
    v->data = (void *)AnalysisData_new();
    v->dtor = (Destructor)AnalysisData_free;

    /* BOILERPLATE: TODO: register analysis callbacks */
    v->previsit_program = Analysis_previsit_program;
    v->postvisit_program = Analysis_postvisit_program;

    v->previsit_vardecl = Analysis_previsit_vardecl;
    v->postvisit_vardecl = Analysis_postvisit_vardecl;

    v->previsit_funcdecl = Analysis_previsit_funcdecl;
    v->postvisit_funcdecl = Analysis_postvisit_funcdecl;

    v->previsit_block = Analysis_previsit_block;
    v->postvisit_block = Analysis_postvisit_block;

    v->previsit_assignment = Analysis_previsit_assignment;
    v->postvisit_assignment = Analysis_postvisit_assignment;

    v->previsit_conditional = Analysis_previsit_conditional;
    v->postvisit_conditional = Analysis_postvisit_conditional;

    v->previsit_whileloop = Analysis_previsit_while_loop;
    v->postvisit_whileloop = Analysis_postvisit_while_loop;

    v->previsit_return = Analysis_previsit_return;
    v->postvisit_return = Analysis_postvisit_return;

    v->previsit_break = Analysis_previsit_break;
    v->postvisit_break = Analysis_postvisit_break;

    v->previsit_continue = Analysis_previsit_continue;
    v->postvisit_continue = Analysis_postvisit_continue;

    v->previsit_binaryop = Analysis_previsit_binop;
    v->invisit_binaryop = Analysis_invisit_binop;
    v->postvisit_binaryop = Analysis_postvisit_binop;

    v->previsit_unaryop = Analysis_previsit_unop;
    v->postvisit_unaryop = Analysis_postvisit_unop;

    v->previsit_location = Analysis_previsit_location;
    v->postvisit_location = Analysis_postvisit_location;

    v->previsit_funccall = Analysis_previsit_funcall;
    v->postvisit_funccall = Analysis_postvisit_funcall;

    v->previsit_literal = Analysis_previsit_literal;
    v->postvisit_literal = Analysis_postvisit_literal;

    // assign a tyoe to all the literals according to chart thing

    /* perform analysis, save error list, clean up, and return errors */
    NodeVisitor_traverse(v, tree);
    ErrorList *errors = ((AnalysisData *)v->data)->errors;
    NodeVisitor_free(v);
    return errors;
}
