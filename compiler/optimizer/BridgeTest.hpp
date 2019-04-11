#ifndef BRIDGE_TEST_INC
#define BRIDGE_TEST_INC
#pragma once


#include "optimizer/Optimization.hpp" // for optimization
#include "optimizer/OptimizationManager.hpp" // for optimization manager


namespace OMR {
   class BridgeTest : public TR::Optimization {
      public:
         BridgeTest(TR::OptimizationManager *m) : TR::Optimization(m) {};
         static TR::Optimization *create (TR::OptimizationManager *m) {
            return new (m->allocator()) BridgeTest(m);
         };
         virtual bool shouldPerform();
         virtual int32_t perform();
         virtual const char * optDetailString() const throw()
         {
            return "O^O Hello World Opt : ";
         };
   };
}
#endif
