#include <boost/test/unit_test.hpp>

#include "../lib/regex/buffered_char_input.hpp"

BOOST_AUTO_TEST_CASE(buffered_char_input_basic)
{
  std::istringstream iss("abcdef");
  BufferedCharInput bci(iss);
  BufferedCharInput::iterator it(bci.begin());

  BOOST_CHECK(it == bci.begin());
  BOOST_CHECK(it != bci.end());

  BOOST_CHECK(*it == 'a');
  BOOST_CHECK(*(it+1) == 'b');

  std::string str;
  while(it != bci.end())
    str += *(it++);
  
  BOOST_CHECK(str == "abcdef");
}

BOOST_AUTO_TEST_CASE(buffered_char_input_operators)
{
  std::istringstream iss("abcdef");
  BufferedCharInput bci(iss);
  BufferedCharInput::iterator it(bci.begin());
  const BufferedCharInput::iterator end(bci.end());
  const BufferedCharInput::iterator middle(it + 3);
  const BufferedCharInput::iterator free_end;

  BOOST_CHECK(it < end);
  BOOST_CHECK(it <= end);

  BOOST_CHECK(it < middle);
  BOOST_CHECK(it <= middle);
  
  BOOST_CHECK(it != end);
  BOOST_CHECK(it != middle);

  BOOST_CHECK(end != it);
  BOOST_CHECK(end != middle);
  
  BOOST_CHECK(end == free_end);
  BOOST_CHECK(not (end != free_end));
  
  BOOST_CHECK(it + 5 != end);
  BOOST_CHECK(it + 6 == end);
  BOOST_CHECK(it + 7 == end);

  ++it;

  BOOST_CHECK(it == bci.begin() + 1);
  BOOST_CHECK(*it == *(bci.begin() + 1));

  BOOST_CHECK(++bci.end() == free_end);
}

BOOST_AUTO_TEST_CASE(buffered_char_input_discard)
{
  std::istringstream iss("abcdef");
  BufferedCharInput bci(iss);
  BufferedCharInput::iterator i1(bci.begin());
  BufferedCharInput::iterator i2(i1 + 1);

  std::string str(i1, i2);
  i1 = bci.discard(i2);
  
  BOOST_CHECK(i1 == bci.begin());
  BOOST_CHECK(str == "a");

  
  std::string str2(i1, bci.end());
  BOOST_CHECK(str2 == "bcdef");
}

BOOST_AUTO_TEST_CASE(istream_test)
{
  std::istringstream iss("ab");

  char c(0);
  iss.get(c);
  iss.get(c);
  BOOST_CHECK(iss.good());
  iss.get(c);
  BOOST_CHECK(iss.eof());
}
