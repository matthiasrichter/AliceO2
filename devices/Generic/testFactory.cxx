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

//AliceO2::Factory<TestBase, const char*>::mInstance = nullptr;

int main()
{
  AliceO2::Factory<TestBase, const char*> factory;

  factory.Register("TestClass1", CreateTestClass1);
  factory.Register("TestClass2", CreateTestClass2);

  TestBase* obj = factory.Create("TestClass1");
  if (obj) obj->print();
  obj = factory.Create("TestClass2");
  if (obj) obj->print();

  return 0;
}
