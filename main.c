#include <stdio.h>
#include <stdlib.h>
#include <math.h>

double rng()
{
    static unsigned int seed = 123*16807;
    seed *= 16807;
 
    return seed / (double)0xFFFFFFFFULL;
}

/*
     1234 / 2 = 0617 + 0/2
    -0      // 0*2
    =12     
    -12     // 6*2
    = 03
    -  2    // 1*2
    =  14
    -  14   // 0*2
    =   0
*/

/*
     1234 / 16 = 0077 + 2/16
    -0      // 0*16
    =12     
    - 0     // 0*16
    =123
    -112    // 7*16
    = 114
    - 112   // 7*16
    =   2
*/

typedef struct Number 
{
    int *digits;
    int base;
    int num_digits;
} Number;


Number base_to_base(Number from_number, int from_base, int to_base)
{
    // copy to perform manual long division in-place
    int copy[from_number.num_digits];
    for (int i = 0; i < from_number.num_digits; i++)
        copy[i] = from_number.digits[i];

    // estimate the maximum number of digits in the converted number and allocate array to store the converted digits
    int max_to_digits = ceil(from_number.num_digits/(log(to_base)/log(from_base)));
    int tmp_to_number[max_to_digits];
    
    int to_digits = 0; // number of digits in the converted base
    while (1) {
        // manual long-division:
        // quotient stored in-place in "copy"
        // remainder stored in "accum"
        int accum = copy[0];
        for (int i = 0; i < from_number.num_digits; i++) {
            int d = accum/to_base;
            copy[i] = d;
            accum -= d*to_base;

            if (i < from_number.num_digits-1) {
                accum *= from_base;
                accum += copy[i+1];
            }
        }
        tmp_to_number[to_digits++] = accum;

        // done if all zeros
        int num_zeros = 0;
        for (int i = 0; i < from_number.num_digits; i++) {
            num_zeros += (copy[i] == 0);
        }

        if (num_zeros == from_number.num_digits)
            break;
    }

    // converted digits are calculated in reverse order, so need to swap them
    Number to_number;
    to_number.num_digits = to_digits;
    to_number.base = to_base;
    to_number.digits = (int*)malloc(4*to_digits); // NB: LEAK!
    for (int i = to_digits-1; i >= 0; i--) {
        to_number.digits[to_digits-1-i] = tmp_to_number[i];
    }
    return to_number;
}



char *encode_number(Number number)
{
    static char chars_hex[] = "0123456789ABCDEF";
    static char chars_base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";  // RFC 1421 compliant, no padding

    if (number.base > 64) {
        printf("Error: Cannot encode bases larger than 64\n");
        return NULL;   
    }

    char *str = (char*)malloc(number.num_digits+1); // NB: LEAK!

    if (number.base <= 16) {
        for (int i = 0; i < number.num_digits; i++) {
            str[i] = chars_hex[number.digits[i]];
        }
    } else if (number.base <= 64) {
        for (int i = 0; i < number.num_digits; i++) {
            str[i] = chars_base64[number.digits[i]];
        }
    }
    str[number.num_digits] = '\0';

    return str;
}

int compare_numbers(Number n1, Number n2)
{
    if (n1.base !=  n2.base || n1.num_digits != n2.num_digits)
        return 0;
    for (int i = 0; i < n1.num_digits; i++) {
        if (n1.digits[i] != n2.digits[i])
            return 0;
    }

    return 1;
}

int main()
{
    {
        // test a small integer that fits in an int32 (~8 digits)
        Number decimal_number;
        decimal_number.num_digits = 9;
        decimal_number.base = 10;
        decimal_number.digits = (int*)malloc(4*decimal_number.num_digits); // NB: LEAK!
        for (int i = 0; i < decimal_number.num_digits; i++) {
            decimal_number.digits[i] = 1 + (i % 9);
        }

        Number binary_number = base_to_base(decimal_number, 10, 2);
        Number hex_number = base_to_base(decimal_number, 10, 16);
        Number octal_number = base_to_base(decimal_number, 10, 8);
        Number base64_number = base_to_base(decimal_number, 10, 64);


        printf("(%s)_%d = (%s)_%d \n\
               = (%s)_%d \n\
               = (%s)_%d \n\
               = (%s)_%d\n\n", encode_number(decimal_number), decimal_number.base, 
                               encode_number(binary_number), binary_number.base,
                               encode_number(octal_number), octal_number.base,
                               encode_number(base64_number), base64_number.base,
                               encode_number(hex_number), hex_number.base); // NB: leak!

    }

    {
        // test a large integer that definitely does not fit in int64 (~16 digits)
        Number decimal_number;
        decimal_number.num_digits = 27;
        decimal_number.base = 10;
        decimal_number.digits = (int*)malloc(4*decimal_number.num_digits); // NB: LEAK!
        for (int i = 0; i < decimal_number.num_digits; i++) {
            decimal_number.digits[i] = 1 + (i % 9);
        }

        Number binary_number = base_to_base(decimal_number, 10, 2);
        Number hex_number = base_to_base(decimal_number, 10, 16);
        Number octal_number = base_to_base(decimal_number, 10, 8);
        Number base64_number = base_to_base(decimal_number, 10, 64);

        printf("(%s)_%d = (%s)_%d \n\
                                 = (%s)_%d \n\
                                 = (%s)_%d \n\
                                 = (%s)_%d\n\n", encode_number(decimal_number), decimal_number.base, 
                                                 encode_number(binary_number), binary_number.base,
                                                 encode_number(octal_number), octal_number.base,
                                                 encode_number(base64_number), base64_number.base,
                                                 encode_number(hex_number), hex_number.base); // NB: leak!
    }

    
    int success = 1;
    // test to see if conversion from decimal to some base and back again gives the exact same string:
    for (long int i = 1; i < 100000; i++) {
        Number decimal_number;
        decimal_number.base = 10;
        decimal_number.num_digits = (int)(log10(i))+1;
        decimal_number.digits = (int*)malloc(4*decimal_number.num_digits);
        int j = i;
        for (int i = 0; i < decimal_number.num_digits; i++) {
            int p = pow(10, decimal_number.num_digits-1-i);
            decimal_number.digits[i] = j/p;
            j  = j % p;
        }

        Number binary_number = base_to_base(decimal_number, 10, 2);
        Number hex_number    = base_to_base(decimal_number, 10, 16);
        Number octal_number  = base_to_base(decimal_number, 10, 8);
        Number base64_number = base_to_base(decimal_number, 10, 64);

        Number decimal_from_binary_number = base_to_base(binary_number, 2, 10);
        Number decimal_from_hex_number    = base_to_base(hex_number, 16, 10);
        Number decimal_from_octal_number  = base_to_base(octal_number, 8, 10);
        Number decimal_from_base64_number = base_to_base(base64_number, 64, 10);


        if (!compare_numbers(decimal_number, decimal_from_binary_number) || 
            !compare_numbers(decimal_number, decimal_from_hex_number) ||
            !compare_numbers(decimal_number, decimal_from_octal_number) || 
            !compare_numbers(decimal_number, decimal_from_base64_number)) {
            printf("%s %s %s %s\n", encode_number(decimal_from_binary_number), 
                                    encode_number(decimal_from_hex_number), 
                                    encode_number(decimal_from_octal_number), 
                                    encode_number(decimal_from_base64_number)); // NB: LEAK!
            success = 0;
        }

        free(decimal_number.digits);

        free(binary_number.digits);
        free(hex_number.digits);
        free(octal_number.digits);
        free(base64_number.digits);

        free(decimal_from_binary_number.digits);
        free(decimal_from_hex_number.digits);
        free(decimal_from_octal_number.digits);
        free(decimal_from_base64_number.digits);


    }
    printf("Test: %s\n", success ? "OK" : "FAIL");

}