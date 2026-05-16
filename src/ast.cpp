#include "ast.hpp"
#include <utility>

ASTNode::ASTNode(ASTKind kind, SourceLoc loc) : kind(kind), loc(loc) {}

void ASTNode::pad(std::ostream& os, int indent) {
    for (int i = 0; i < indent; ++i) os << ' ';
}

TypeNode::TypeNode(ASTKind kind, SourceLoc loc) : ASTNode(kind, loc) {}

TypeNameNode::TypeNameNode(std::string name, SourceLoc loc) : TypeNode(ASTKind::TypeName, loc), name(std::move(name)) {}

void TypeNameNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "TypeName(" << name << ")\n";
}

RangeTypeNode::RangeTypeNode(ASTPtr low, ASTPtr high, SourceLoc loc) : TypeNode(ASTKind::RangeType, loc), low(std::move(low)), high(std::move(high)) {}

void RangeTypeNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "RangeType\n";
    pad(os, indent + 2);
    os << "low:\n";
    if (low) low->print(os, indent + 4);
    pad(os, indent + 2);
    os << "high:\n";
    if (high) high->print(os, indent + 4);
}

EnumTypeNode::EnumTypeNode(std::vector<std::string> names, SourceLoc loc) : TypeNode(ASTKind::EnumType, loc), names(std::move(names)) {}

void EnumTypeNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "EnumType(";
    for (size_t i = 0; i < names.size(); ++i) {
        if (i > 0) os << ", ";
        os << names[i];
    }
    os << ")\n";
}

ArrayTypeNode::ArrayTypeNode(ASTPtr indexType, ASTPtr elementType, SourceLoc loc) : TypeNode(ASTKind::ArrayType, loc), indexType(std::move(indexType)), elementType(std::move(elementType)) {}

void ArrayTypeNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "ArrayType\n";
    pad(os, indent + 2);
    os << "index:\n";
    if (indexType) indexType->print(os, indent + 4);
    pad(os, indent + 2);
    os << "element:\n";
    if (elementType) elementType->print(os, indent + 4);
}

RecordTypeNode::RecordTypeNode(std::vector<ASTPtr> fields, SourceLoc loc) : TypeNode(ASTKind::RecordType, loc), fields(std::move(fields)) {}

void RecordTypeNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "RecordType\n";
    for (const auto& field : fields) {
        field->print(os, indent + 2);
    }
}

ConstDeclNode::ConstDeclNode(std::string name, ASTPtr value, SourceLoc loc) : ASTNode(ASTKind::ConstDecl, loc), name(std::move(name)), value(std::move(value)) {}

void ConstDeclNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "ConstDecl(name: '" << name << "')\n";
    if (value) value->print(os, indent + 2);
}

TypeDeclNode::TypeDeclNode(std::string name, ASTPtr typeNode, SourceLoc loc) : ASTNode(ASTKind::TypeDecl, loc), name(std::move(name)), typeNode(std::move(typeNode)) {}

void TypeDeclNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "TypeDecl(name: '" << name << "')\n";
    if (typeNode) typeNode->print(os, indent + 2);
}

VarDeclNode::VarDeclNode(std::string name, ASTPtr typeNode, SourceLoc loc) : ASTNode(ASTKind::VarDecl, loc), name(std::move(name)), typeNode(std::move(typeNode)) {}

void VarDeclNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "VarDecl(name: '" << name << "')\n";
    if (typeNode) typeNode->print(os, indent + 2);
}

ParamDeclNode::ParamDeclNode(std::string name, ASTPtr typeNode, bool byReference, SourceLoc loc) : ASTNode(ASTKind::ParamDecl, loc), name(std::move(name)), typeNode(std::move(typeNode)), byReference(byReference) {}

void ParamDeclNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "ParamDecl(name: '" << name
       << "', mode: '" << (byReference ? "var" : "value") << "')\n";
    if (typeNode) typeNode->print(os, indent + 2);
}

EmptyNode::EmptyNode(SourceLoc loc) : ASTNode(ASTKind::Empty, loc) {}

void EmptyNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "Empty\n";
}

ProgramNode::ProgramNode(std::string name, std::vector<ASTPtr> declarations, ASTPtr block, SourceLoc loc) : ASTNode(ASTKind::Program, loc), name(std::move(name)), declarations(std::move(declarations)), block(std::move(block)) {}

void ProgramNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "ProgramNode(name: '" << name << "')\n";

    pad(os, indent + 2);
    os << "Declarations\n";
    for (const auto& decl : declarations) decl->print(os, indent + 4);

    pad(os, indent + 2);
    os << "Block\n";
    if (block) block->print(os, indent + 4);
}

BlockNode::BlockNode(std::vector<ASTPtr> statements, SourceLoc loc) : ASTNode(ASTKind::Block, loc), statements(std::move(statements)) {}

void BlockNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "BlockNode\n";
    for (const auto& stmt : statements) stmt->print(os, indent + 2);
}

ProcedureDeclNode::ProcedureDeclNode(
    std::string name,
    std::vector<ASTPtr> params,
    std::vector<ASTPtr> declarations,
    ASTPtr block,
    SourceLoc loc
) : ASTNode(ASTKind::ProcedureDecl, loc),
      name(std::move(name)),
      params(std::move(params)),
      declarations(std::move(declarations)),
      block(std::move(block)) {}

void ProcedureDeclNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "ProcedureDecl(name: '" << name << "')\n";

    pad(os, indent + 2);
    os << "Params\n";
    for (const auto& param : params) param->print(os, indent + 4);

    pad(os, indent + 2);
    os << "Declarations\n";
    for (const auto& decl : declarations) decl->print(os, indent + 4);

    pad(os, indent + 2);
    os << "Block\n";
    if (block) block->print(os, indent + 4);
}

FunctionDeclNode::FunctionDeclNode(
    std::string name,
    std::vector<ASTPtr> params,
    ASTPtr returnType,
    std::vector<ASTPtr> declarations,
    ASTPtr block,
    SourceLoc loc
) : ASTNode(ASTKind::FunctionDecl, loc),
      name(std::move(name)),
      params(std::move(params)),
      returnType(std::move(returnType)),
      declarations(std::move(declarations)),
      block(std::move(block)) {}

void FunctionDeclNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "FunctionDecl(name: '" << name << "')\n";

    pad(os, indent + 2);
    os << "Params\n";
    for (const auto& param : params) param->print(os, indent + 4);

    pad(os, indent + 2);
    os << "ReturnType\n";
    if (returnType) returnType->print(os, indent + 4);

    pad(os, indent + 2);
    os << "Declarations\n";
    for (const auto& decl : declarations) decl->print(os, indent + 4);

    pad(os, indent + 2);
    os << "Block\n";
    if (block) block->print(os, indent + 4);
}

VarNode::VarNode(std::string name, SourceLoc loc) : ASTNode(ASTKind::Var, loc), name(std::move(name)) {}

void VarNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "Var('" << name << "')\n";
}

ArrayAccessNode::ArrayAccessNode(ASTPtr base, std::vector<ASTPtr> indices, SourceLoc loc) : ASTNode(ASTKind::ArrayAccess, loc), base(std::move(base)), indices(std::move(indices)) {}

void ArrayAccessNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "ArrayAccess\n";
    pad(os, indent + 2);
    os << "base:\n";
    if (base) base->print(os, indent + 4);
    pad(os, indent + 2);
    os << "indices:\n";
    for (const auto& index : indices) index->print(os, indent + 4);
}

FieldAccessNode::FieldAccessNode(ASTPtr base, std::string fieldName, SourceLoc loc) : ASTNode(ASTKind::FieldAccess, loc), base(std::move(base)), fieldName(std::move(fieldName)) {}

void FieldAccessNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "FieldAccess(field: '" << fieldName << "')\n";
    if (base) base->print(os, indent + 2);
}

NumNode::NumNode(std::string rawValue, bool isReal, SourceLoc loc) : ASTNode(ASTKind::Num, loc), rawValue(std::move(rawValue)), isReal(isReal) {}

void NumNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << (isReal ? "Real" : "Integer") << "Number(" << rawValue << ")\n";
}

StringNode::StringNode(std::string value, SourceLoc loc) : ASTNode(ASTKind::String, loc), value(std::move(value)) {}

void StringNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "String('" << value << "')\n";
}

CharNode::CharNode(std::string value, SourceLoc loc) : ASTNode(ASTKind::Char, loc), value(std::move(value)) {}

void CharNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "Char('" << value << "')\n";
}

BooleanNode::BooleanNode(bool value, SourceLoc loc) : ASTNode(ASTKind::Boolean, loc), value(value) {}

void BooleanNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "Boolean(" << (value ? "true" : "false") << ")\n";
}

BinOpNode::BinOpNode(std::string op, ASTPtr left, ASTPtr right, SourceLoc loc) : ASTNode(ASTKind::BinOp, loc), op(std::move(op)), left(std::move(left)), right(std::move(right)) {}

void BinOpNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "BinOp('" << op << "')\n";
    if (left) left->print(os, indent + 2);
    if (right) right->print(os, indent + 2);
}

UnaryOpNode::UnaryOpNode(std::string op, ASTPtr operand, SourceLoc loc) : ASTNode(ASTKind::UnaryOp, loc), op(std::move(op)), operand(std::move(operand)) {}

void UnaryOpNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "UnaryOp('" << op << "')\n";
    if (operand) operand->print(os, indent + 2);
}

AssignNode::AssignNode(ASTPtr target, ASTPtr value, SourceLoc loc) : ASTNode(ASTKind::Assign, loc), target(std::move(target)), value(std::move(value)) {}

void AssignNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "Assign\n";

    pad(os, indent + 2);
    os << "target:\n";
    if (target) target->print(os, indent + 4);

    pad(os, indent + 2);
    os << "value:\n";
    if (value) value->print(os, indent + 4);
}

ProcedureCallNode::ProcedureCallNode(std::string name, std::vector<ASTPtr> args, SourceLoc loc) : ASTNode(ASTKind::ProcedureCall, loc), name(std::move(name)), args(std::move(args)) {}

void ProcedureCallNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "ProcedureCall(name: '" << name << "')\n";
    for (const auto& arg : args) arg->print(os, indent + 2);
}

IfNode::IfNode(ASTPtr condition, ASTPtr thenBranch, ASTPtr elseBranch, SourceLoc loc)
    : ASTNode(ASTKind::IfStatement, loc),
      condition(std::move(condition)),
      thenBranch(std::move(thenBranch)),
      elseBranch(std::move(elseBranch)) {}

void IfNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "If\n";

    pad(os, indent + 2);
    os << "condition:\n";
    if (condition) condition->print(os, indent + 4);

    pad(os, indent + 2);
    os << "then:\n";
    if (thenBranch) thenBranch->print(os, indent + 4);

    if (elseBranch) {
        pad(os, indent + 2);
        os << "else:\n";
        elseBranch->print(os, indent + 4);
    }
}

WhileNode::WhileNode(ASTPtr condition, ASTPtr body, SourceLoc loc) : ASTNode(ASTKind::WhileStatement, loc), condition(std::move(condition)), body(std::move(body)) {}

void WhileNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "While\n";

    pad(os, indent + 2);
    os << "condition:\n";
    if (condition) condition->print(os, indent + 4);

    pad(os, indent + 2);
    os << "body:\n";
    if (body) body->print(os, indent + 4);
}

RepeatNode::RepeatNode(ASTPtr body, ASTPtr condition, SourceLoc loc) : ASTNode(ASTKind::RepeatStatement, loc), body(std::move(body)), condition(std::move(condition)) {}

void RepeatNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "Repeat\n";

    pad(os, indent + 2);
    os << "body:\n";
    if (body) body->print(os, indent + 4);

    pad(os, indent + 2);
    os << "until:\n";
    if (condition) condition->print(os, indent + 4);
}

ForNode::ForNode(
    std::string iterName,
    ASTPtr startExpr,
    ASTPtr endExpr,
    bool isDownTo,
    ASTPtr body,
    SourceLoc loc
) : ASTNode(ASTKind::ForStatement, loc),
      iterName(std::move(iterName)),
      startExpr(std::move(startExpr)),
      endExpr(std::move(endExpr)),
      isDownTo(isDownTo),
      body(std::move(body)) {}

void ForNode::print(std::ostream& os, int indent) const {
    pad(os, indent);
    os << "For(iterator: '" << iterName
       << "', direction: '" << (isDownTo ? "isDownTo" : "to") << "')\n";

    pad(os, indent + 2);
    os << "start:\n";
    if (startExpr) startExpr->print(os, indent + 4);

    pad(os, indent + 2);
    os << "end:\n";
    if (endExpr) endExpr->print(os, indent + 4);

    pad(os, indent + 2);
    os << "body:\n";
    if (body) body->print(os, indent + 4);
}