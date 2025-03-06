#include "../include/utils.h"

int main(int argc, char *argv[]) {
  try {
    if (argc < 3 || argc > 3) {
      throw std::runtime_error{
          "Необходимо указать два аргумента: входной файл и выходной файл"};
    }

    Parser fileParser(argv[1]);
    std::ofstream outputFile(argv[2]);

    auto parsedFunction = fileParser.Parse();
    CodeRunner coderunner(std::move(parsedFunction));

    if (!outputFile.is_open()) {
      throw std::runtime_error{"Не удалось открыть выходной файл"};
    }

    coderunner.Run(outputFile);

  } catch (const std::runtime_error &error) {
    std::cerr << "Ошибка: " << error.what() << std::endl;
    return 1;
  }

  return 0;
}
