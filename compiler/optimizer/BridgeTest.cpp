#include "optimizer/BridgeTest.hpp"

#include "env/VerboseLog.hpp"
#include "compiler/il/Block.hpp"
#include "compiler/il/SymbolReference.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <map>

// Anonymous namespace to hold file static functions.
namespace {

void log(const char *msg) {
  TR_VerboseLog::vlogAcquire();
  TR_VerboseLog::writeLine(TR_Vlog_BT, msg);
  TR_VerboseLog::vlogRelease();
}

void log(int32_t i) {
  char num[5] = { 0 };

  sprintf(num, "%d", i);
  log(num);
}

void dlog(const char *msg) {
  fprintf(stderr, "---- %s ----\n", msg);
}

uint32_t recurseC(TR::Node *node) {
  int32_t childCount = node->getNumChildren();
  uint32_t acc = 0;
  for (int32_t i = 0; i < childCount; ++i) {
    acc += recurseC(node->getChild(i));
  }

  return acc + childCount;
}

}

OMR::BridgeTest::BridgeTest(TR::OptimizationManager *m)
  : TR::Optimization(m),
  ids(comp()->aliasRegion()), blocks(comp()->aliasRegion()),
  codeFile(nullptr),
  valueCount(0), blockCount(0),
  bridgeDebug(false) { };

OMR::BridgeTest::~BridgeTest() {
  if (codeFile)
    fclose(codeFile);

  char str[256] = { 0 };
  sprintf(str, "%lu", ids.size());
  log(str);
}

bool
OMR::BridgeTest::shouldPerform() {
  const char *name = comp()->getMethodSymbol()->getMethod()->nameChars();

  bool doIt = false;
  if (strcmp("sleepLoop", name) == 0) {
    doIt = true;
    log("Doing it!");
  }
  else {
    doIt = false;
    log("Skipping");
  }

  log(name);
  return doIt;
}

int32_t
OMR::BridgeTest::perform() {
  // Make code file.
  codeFile = fopen("bridgeCode.txt", "w");

  // Get symbol.
  TR::ResolvedMethodSymbol *sym = comp()->getMethodSymbol();
  TR_ResolvedMethod *method = sym->getResolvedMethod();

  // Get intermediate strings... Damn tr and non-null terminated strings.
  char className[1024] = { 0 };
  char name[1024] = { 0 };
  char sig[1024] = { 0 };
  strncpy(className, method->classNameChars(), method->classNameLength());
  strncpy(name, method->nameChars(), method->nameLength());
  strncpy(sig, method->signatureChars(), method->signatureLength());

  // Getting signature.
  char sigLine[4096] = { 0 };
  sprintf(
    sigLine, "%s:%s:%s",
    className, name, sig
  );
  writeToCodeFile(sigLine, false, false);
  dlog(sigLine);

  // Traverse the tree.
  traverseTrees();

  return 0;
}


uint32_t depth = 0;
void OMR::BridgeTest::visitNode(TR::Node *node) {
  // Visit children first, left-to-right.
  int32_t childCount = node->getNumChildren();
  ++depth;
  for (int32_t i = 0; i < childCount; ++i) {
    visitNode(node->getChild(i));
  }
  --depth;

  // Get id.
  uint32_t id;
  bool seen;
  if (!idsContains(node)) {
    id = valueCount;
    addId(node, valueCount++);
    seen = false;
  }
  else {
    id = getId(node);
    seen = true;
  }

  // Common string space.
  char instStr[4096] = { 0 };

  // Should skip this inst? Assume no. Change if necessary in switch.
  bool skip = false;
  switch (node->getOpCodeValue()) {
  case TR::iconst: {
    assert(childCount == 0 && "iconst had children.");
    assert(node->canGet32bitIntegralValue() && "iconst not 32bits.");
    sprintf(
      instStr, "%u:iconst:%s:%d",
      id, OMR::DataType::getName(node->getDataType()),
      node->get32bitIntegralValue()
    );
    break;
  }
  case TR::lconst: {
    assert(childCount == 0 && "lconst had children.");
    assert(node->canGet64bitIntegralValue() && "lconst not 64bits.");
    sprintf(
      instStr, "%u:lconst:%s:%ld",
      id, OMR::DataType::getName(node->getDataType()),
      node->get64bitIntegralValue()
    );
    break;
  }
  case TR::aload: {
    assert(node->getOpCode().hasSymbolReference() && "aload did not have symbol reference.");
    sprintf(
      instStr, "%u:aload:%s:%s",
      id, OMR::DataType::getName(node->getDataType()),
//      comp()->getDebug()->getName(node->getSymbolReference())
      node->getSymbolReference()->getName(comp()->getDebug())
    );
    break;
  }
  case TR::iloadi: {
    assert(node->getOpCode().hasSymbolReference() && "iloadi did not have symbol reference.");
    assert(childCount == 1 && "iloadi did not have 1 child.");
    sprintf(
      instStr, "%u:iloadi:%s:%u:%s",
      id, OMR::DataType::getName(node->getDataType()),
      getId(node->getChild(0)),
      node->getSymbolReference()->getName(comp()->getDebug())
    );
    break;
  }
  case TR::aloadi: {
    assert(node->getOpCode().hasSymbolReference() && "aloadi did not have symbol reference.");
    assert(childCount == 1 && "aloadi did not have 1 child.");
    sprintf(
      instStr, "%u:aloadi:%s:%u:%s",
      id, OMR::DataType::getName(node->getDataType()),
      getId(node->getChild(0)),
      node->getSymbolReference()->getName(comp()->getDebug())
    );
    break;
  }
  case TR::istorei: {
    assert(node->getOpCode().hasSymbolReference() && "istorei did not have symbol reference.");
    assert(childCount == 2 && "istorei did not have 2 children.");
    assert(node->getDataType() == node->getChild(1)->getDataType() &&
           "istorei store type did not match types.");
    sprintf(
      instStr, "%u:istorei:%s:%u:%u:%s",
      id, OMR::DataType::getName(node->getDataType()),
      getId(node->getChild(0)), getId(node->getChild(1)),
      node->getSymbolReference()->getName(comp()->getDebug())
    );
    break;
  }
  case TR::Goto: {
    assert(node->getDataType() == TR::NoType && "goto has a data type.");
    assert(childCount == 0 && "goto had children.");
    sprintf(
      instStr, "%u:goto:%d",
      id,
      node->getBranchDestination()->getNode()->getBlock()->getNumber()
    );
    break;
  }
  case TR::Return: {
    assert(node->getDataType() == TR::NoType && "return has a data type.");
    assert(childCount == 0 && "return had children.");
    sprintf(instStr, "%u:return", id);
    break;
  }
  case TR::asynccheck: {
    // Docs claim this is a GC point. Do we care?
    assert(node->getDataType() == TR::NoType && "asynccheck has a data type.");
    assert(childCount == 0 && "asynccheck had children.");
    sprintf(instStr, "%u:asynccheck", id);
    skip = true;
    break;
  }
  case TR::iand: {
    assert(childCount == 2 && "iand did not have 2 children.");
    sprintf(
      instStr, "%u:iand:%s:%u:%u",
      id, OMR::DataType::getName(node->getDataType()),
      getId(node->getChild(0)), getId(node->getChild(1))
    );
    break;
  }
  case TR::ificmpeq: {
    assert(node->getDataType() == TR::NoType && "ificmpeq has a data type.");
    assert(childCount == 2 && "ificmpeq did not have 2 children.");
    sprintf(
      instStr, "%u:ificmpeq:%u:%u",
      id,
      getId(node->getChild(0)), getId(node->getChild(1))
    );
    break;
  }
  case TR::compressedRefs: {
    // Can probably be skipped.
    assert(node->getDataType() == TR::Address && "compressedrefs did not have address type.");
    assert(childCount == 2 && "compressedrefs did not have 2 children.");
    sprintf(
      instStr, "%u:compressedRefs:%s:%u:%u",
      id, OMR::DataType::getName(node->getDataType()),
      getId(node->getChild(0)), getId(node->getChild(1))
    );
    skip = true;
    break;
  }
  case TR::BBStart: {
    int32_t blockNumber = node->getBlock()->getNumber();
    sprintf(
      instStr, "%d:bbs",
      node->getBlock()->getNumber()
    );
    break;
  }
  case TR::BBEnd: {
    sprintf(
      instStr, "%d:bbe",
      node->getBlock()->getNumber()
    );
    break;
  }
  case TR::calli: {
    assert(node->getDataType() == TR::NoType && "calli has a data type.");
    assert(childCount == 2 && "calli did not have 2 children.");
    assert(node->getChild(0)->getDataType() == TR::Address && "calli child not address.");

    // Getting method signature.
    TR::ResolvedMethodSymbol *sym = node->getSymbol()->getResolvedMethodSymbol();
    assert(sym && "calli resolved method symbol was null.");

    // Getting method.
    TR_ResolvedMethod *method = sym->getResolvedMethod();

    // Get intermediate strings... Damn tr and non-null terminated strings.
    char className[1024] = { 0 };
    char name[1024] = { 0 };
    char sig[1024] = { 0 };
    strncpy(className, method->classNameChars(), method->classNameLength());
    strncpy(name, method->nameChars(), method->nameLength());
    strncpy(sig, method->signatureChars(), method->signatureLength());

    sprintf(
      instStr, "%u:calli:%s:%u:%u:%s:%s:%s",
      id, OMR::DataType::getName(node->getDataType()),
      getId(node->getChild(0)), getId(node->getChild(1)),
      className, name, sig
    );
    break;
  }
  case TR::NULLCHK: {
    assert(node->getDataType() == TR::Address && "NULLCHK did not have address type.");
    assert(childCount == 1 && "NULLCHK did not have 1 child.");
    sprintf(
      instStr, "%u:NULLCHK:%s:%u",
      id, OMR::DataType::getName(node->getDataType()),
      getId(node->getChild(0))
    );
    //skip = true;
    break;
  }
  case TR::ResolveCHK: {
    // Can probably be skipped.
    assert(node->getDataType() == TR::Address && "ResolveCHK did not have address type.");
    assert(childCount == 1 && "ResolveCHK did not have 1 child.");
    sprintf(
      instStr, "%u:ResolveCHK:%s:%u",
      id, OMR::DataType::getName(node->getDataType()),
      getId(node->getChild(0))
    );
    //skip = true;
    break;
  }
  default: {
    sprintf(instStr, "!!!!!!!!default %d %s", depth, node->getOpCode().getName());
    break;
  }
  }

  // Dumping.
  writeToCodeFile(instStr, skip, seen);

  // Logging.
  char dbgStr[1050];
  if (seen)
    sprintf(dbgStr, "%u //%s", depth, instStr);
  else
    sprintf(dbgStr, "%u %s", depth, instStr);
  dlog(dbgStr);
}

void OMR::BridgeTest::traverseTrees() {
  // Get symbol.
  TR::ResolvedMethodSymbol * sym = comp()->getMethodSymbol();
  TR::TreeTop *tree = sym->getFirstTreeTop();

  while(tree) {
    TR::Node *node = tree->getNode();
    node->printFullSubtree();
    visitNode(node);

    tree = tree->getNextTreeTop();
  }
}

bool OMR::BridgeTest::idsContains(TR::Node *node) {
  for (const idPair &pair : ids)
    if (pair.first == node)
      return true;
  return false;
}

void OMR::BridgeTest::addId(TR::Node *node, uint32_t id) {
  assert(!idsContains(node) && "Adding an id that exists already.");
  ids.emplace_back(idPair(node, id));
}

uint32_t OMR::BridgeTest::getId(TR::Node *node) {
  assert(idsContains(node) && "Getting id that was not contained.");
  for (const idPair &pair : ids)
    if (pair.first == node)
      return pair.second;

  assert(false && "Couldn't find id despite containing.");
  return -1;
}


//bool OMR::BridgeTest::blocksContains(uint32_t id) {
//  for (const blockPair &pair : blocks)
//    if (pair.first == id)
//      return true;
//  return false;
//}
//
//void OMR::BridgeTest::addBlock(uint32_t id, int32_t block) {
//  assert(!blocksContains(id) && "Adding a block that exists already.");
//  blocks.emplace_back(blockPair(id, block));
//}
//
//int32_t OMR::BridgeTest::getBlock(uint32_t id) {
//  assert(blocksContains(id) && "Getting block that was not contained.");
//  for (const blockPair &pair : blocks)
//    if (pair.first == id)
//      return pair.second;
//
//  assert(false && "Couldn't find block despite containing.");
//  return -1;
//}

void OMR::BridgeTest::writeToCodeFile(const char *str, bool skip, bool seen) {
  // Sanity check.
  assert(codeFile != nullptr && "Code file was null");

  // We should print if bridgeDebug is on or if it has been seen or
  // shouldn't be skipped.
  if (!bridgeDebug && seen)
    return;
  if (!bridgeDebug && skip)
    return;

  // Produce a final string. First case is if debugging is on.
  char finStr[1050];
  if (seen || skip)
    sprintf(finStr, "//%s", str);
  else
    sprintf(finStr, "%s", str);

  // Print to code file.
  fprintf(codeFile, "%s\n", finStr);
}
