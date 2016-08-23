#include "Factory.h"
#include <iostream>

class TestBase
{
public:
  virtual void print() const = 0;
};

class TestClass1 : public TestBase
{
public:
  void print() const {std::cout << "TestClass1" << std::endl;}
};

TestClass1* CreateTestClass1() {return new TestClass1;}

class TestClass2 : public TestBase
{
public:
  void print() const {std::cout << "TestClass2" << std::endl;}
};

TestClass2* CreateTestClass2() {return new TestClass2;}

typedef AliceO2::Factory<TestBase, const char*> TestFactory;

TestFactory::Registrar gCreateTestClass1("TestClass1", CreateTestClass1);
TestFactory::Registrar gCreateTestClass2("TestClass2", CreateTestClass2);

int main()
{
  TestFactory factory;

  TestBase* obj = factory.Create("TestClass1");
  if (obj) obj->print();
  obj = factory.Create("TestClass2");
  if (obj) obj->print();

  return 0;
}
