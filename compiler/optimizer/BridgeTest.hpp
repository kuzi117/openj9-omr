#ifndef BRIDGE_TEST_INC
#define BRIDGE_TEST_INC
#pragma once


#include "optimizer/Optimization.hpp" // for optimization
#include "optimizer/OptimizationManager.hpp" // for optimization manager
#include "compiler/infra/vector.hpp"
#include "compiler/env/Region.hpp"

namespace OMR {

//template <class Key, class T>
//class map : public std::map<Key, T, std::less<Key>, typed_allocator<std::

class BridgeTest : public TR::Optimization {
public:
  BridgeTest(TR::OptimizationManager *m);
  ~BridgeTest();
  static TR::Optimization *create (TR::OptimizationManager *m) {
    return new (m->allocator()) BridgeTest(m);
  }
  virtual bool shouldPerform();
  virtual int32_t perform();
  virtual const char * optDetailString() const throw() {
    return "O^O To WALA bridge.";
  }
private:
  // Traversing trees.
  void traverseTrees();
  void visitNode(TR::Node *node);

  // Map management.
  bool idsContains(TR::Node *node);
  void addId(TR::Node *node, uint32_t);
  uint32_t getId(TR::Node *node);
  //bool blocksContains(uint32_t id);
  //void addBlock(uint32_t id, int32_t block);
  //int32_t getBlock(uint32_t id);

  // Map typedefs.
  typedef std::pair<TR::Node *, uint32_t> idPair;
  typedef std::pair<uint32_t, uint32_t> blockPair;

  // Map data.
  TR::vector<idPair, TR::Region&> ids;
  TR::vector<blockPair, TR::Region&> blocks;

  // Transform data.
  void writeToCodeFile(const char *str, bool skip, bool seen);
  FILE *codeFile;

  // Tracking
  uint32_t valueCount;
  uint32_t blockCount;

  // Debug?
  bool bridgeDebug;
};
}
#endif
