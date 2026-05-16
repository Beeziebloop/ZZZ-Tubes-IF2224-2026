#ifndef AST_HPP
#define AST_HPP

#include <iostream>
#include <memory>
#include <string>
#include <vector>

//jenis-jenis node AST
enum class ASTKind {
    Program,
    Block,
    ConstDecl,
    TypeDecl,
    VarDecl,
    ParamDecl,
    ProcedureDecl,
    FunctionDecl,
    Assign,
    ProcedureCall,
    IfStatement,
    WhileStatement,
    RepeatStatement,
    ForStatement,
    BinOp,
    UnaryOp,
    Var,
    ArrayAccess,
    FieldAccess,
    Num,
    String,
    Char,
    Boolean,
    TypeName,
    ArrayType,
    RecordType,
    RangeType,
    EnumType,
    Empty
};

//lokasi source code
struct SourceLoc {
    int line = -1;
    int column = -1;
};

//base class untuk semua node AST
struct ASTNode {
    ASTKind kind;
    SourceLoc loc;
    std::string inferredType;
    int tabIndex = -1;
    int lexicalLevel = -1;
    explicit ASTNode(ASTKind kind, SourceLoc loc = {});
    virtual ~ASTNode() = default;
    virtual void print(std::ostream& os, int indent = 0) const = 0;
protected:
    static void pad(std::ostream& os, int indent);
};

using ASTPtr = std::unique_ptr<ASTNode>;

struct TypeNode : ASTNode {
    explicit TypeNode(ASTKind kind, SourceLoc loc = {});
};

struct TypeNameNode final : TypeNode {
    std::string name;
    explicit TypeNameNode(std::string name, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct RangeTypeNode final : TypeNode {
    ASTPtr low;
    ASTPtr high;
    RangeTypeNode(ASTPtr low, ASTPtr high, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct EnumTypeNode final : TypeNode {
    std::vector<std::string> names;
    explicit EnumTypeNode(std::vector<std::string> names, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct ArrayTypeNode final : TypeNode {
    ASTPtr indexType;
    ASTPtr elementType;
    ArrayTypeNode(ASTPtr indexType, ASTPtr elementType, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct RecordTypeNode final : TypeNode {
    std::vector<ASTPtr> fields;
    explicit RecordTypeNode(std::vector<ASTPtr> fields, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct ConstDeclNode final : ASTNode {
    std::string name;
    ASTPtr value;
    ConstDeclNode(std::string name, ASTPtr value, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct TypeDeclNode final : ASTNode {
    std::string name;
    ASTPtr typeNode;
    TypeDeclNode(std::string name, ASTPtr typeNode, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct VarDeclNode final : ASTNode {
    std::string name;
    ASTPtr typeNode;
    VarDeclNode(std::string name, ASTPtr typeNode, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct ParamDeclNode final : ASTNode {
    std::string name;
    ASTPtr typeNode;
    bool byReference = false;
    ParamDeclNode(std::string name, ASTPtr typeNode, bool byReference = false, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct EmptyNode final : ASTNode {
    explicit EmptyNode(SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct ProgramNode final : ASTNode {
    std::string name;
    std::vector<ASTPtr> declarations;
    ASTPtr block;
    ProgramNode(std::string name, std::vector<ASTPtr> declarations, ASTPtr block, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct BlockNode final : ASTNode {
    std::vector<ASTPtr> statements;
    explicit BlockNode(std::vector<ASTPtr> statements, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct ProcedureDeclNode final : ASTNode {
    std::string name;
    std::vector<ASTPtr> params;
    std::vector<ASTPtr> declarations;
    ASTPtr block;
    ProcedureDeclNode(std::string name, std::vector<ASTPtr> params, std::vector<ASTPtr> declarations, ASTPtr block, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct FunctionDeclNode final : ASTNode {
    std::string name;
    std::vector<ASTPtr> params;
    ASTPtr returnType;
    std::vector<ASTPtr> declarations;
    ASTPtr block;
    FunctionDeclNode(std::string name, std::vector<ASTPtr> params, ASTPtr returnType, std::vector<ASTPtr> declarations, ASTPtr block, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct VarNode final : ASTNode {
    std::string name;
    explicit VarNode(std::string name, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct ArrayAccessNode final : ASTNode {
    ASTPtr base;
    std::vector<ASTPtr> indices;
    ArrayAccessNode(ASTPtr base, std::vector<ASTPtr> indices, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct FieldAccessNode final : ASTNode {
    ASTPtr base;
    std::string fieldName;
    FieldAccessNode(ASTPtr base, std::string fieldName, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct NumNode final : ASTNode {
    std::string rawValue;
    bool isReal;
    NumNode(std::string rawValue, bool isReal, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct StringNode final : ASTNode {
    std::string value;
    explicit StringNode(std::string value, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct CharNode final : ASTNode {
    std::string value;
    explicit CharNode(std::string value, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct BooleanNode final : ASTNode {
    bool value;
    explicit BooleanNode(bool value, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct BinOpNode final : ASTNode {
    std::string op;
    ASTPtr left;
    ASTPtr right;
    BinOpNode(std::string op, ASTPtr left, ASTPtr right, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct UnaryOpNode final : ASTNode {
    std::string op;
    ASTPtr operand;
    UnaryOpNode(std::string op, ASTPtr operand, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct AssignNode final : ASTNode {
    ASTPtr target;
    ASTPtr value;
    AssignNode(ASTPtr target, ASTPtr value, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct ProcedureCallNode final : ASTNode {
    std::string name;
    std::vector<ASTPtr> args;
    ProcedureCallNode(std::string name, std::vector<ASTPtr> args, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct IfNode final : ASTNode {
    ASTPtr condition;
    ASTPtr thenBranch;
    ASTPtr elseBranch;
    IfNode(ASTPtr condition, ASTPtr thenBranch, ASTPtr elseBranch = nullptr, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct WhileNode final : ASTNode {
    ASTPtr condition;
    ASTPtr body;
    WhileNode(ASTPtr condition, ASTPtr body, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct RepeatNode final : ASTNode {
    ASTPtr body;
    ASTPtr condition;
    RepeatNode(ASTPtr body, ASTPtr condition, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

struct ForNode final : ASTNode {
    std::string iterName;
    ASTPtr startExpr;
    ASTPtr endExpr;
    bool isDownTo = false;
    ASTPtr body;
    ForNode(std::string iterName, ASTPtr startExpr, ASTPtr endExpr, bool isDownTo, ASTPtr body, SourceLoc loc = {});
    void print(std::ostream& os, int indent = 0) const override;
};

#endif