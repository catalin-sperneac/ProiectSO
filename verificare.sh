source_file="$1"  # fisier care trebuie verificat

# verificam numarul de linii, cuvinte si caractere si daca fisierul contine caractere non-ASCII sau cuvinte specifice 
verification() 
{
    # numarul de linii,cuvinte si caractere
    lines=$(wc -l < "$1")
    words=$(wc -w < "$1")
    characters=$(wc -m < "$1")

    if [ "$lines" -lt 3 ] && [ "$words" -gt 1000 ] && [ "$characters" -gt 2000 ]; then
        echo "$1"
        exit 1
    else 
        if grep -qP '[^\x00-\x7F]' "$1" || grep -qE 'corrupted|malicious|malware|dangerous|risk|attack' "$1"; then 
            echo "$1"  
            exit 1
            else
                echo "SAFE"
                exit 0
        fi
    fi
}

chmod 777 "$source_file"
verification "$source_file"
