#include <check.h>
#include <stdlib.h>
#include "money.h"

START_TEST(test_money_create){
    Money *m;
    m = money_create(5, "USD");
    ck_assert_int_eq(money_amount(m), 5);
    ck_assert_str_eq(money_currency(m), "USD");
    money_free(m);

}
END_TEST
Suite *money_suite(void)
{
    Suite *s;//структура для содержания набров тестов
    TCase *tc_core;//указатель на структуру содержащую тестовый пример
    s = suite_create("Money");//создание наборов тестов
    tc_core = tcase_create("Core");//создание тестового примера
    tcase_add_test(tc_core, test_money_create);//заполнение тестового набора путем вызова tcase_create и тестовой функции
    suite_add_tcase(s, tc_core);//заполнение набора тестов s тестовыми примерами
    return s;///указатель который содержит набор тестов

}
int main(void){
    return 0;
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = money_suite();
    sr = srunner_create(s);//создается эземпляр SRunner который хранит состояние выполненых тестов
    srunner_run_all(sr, CK_NORMAL);//запуск всех тестов
    number_failed = srunner_ntests_failed(sr);//подсчет количества неудачных тестов
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;// E_S возвращется если все тесты прошли E_F возвращает если какие нибудь тесты не прошли
}