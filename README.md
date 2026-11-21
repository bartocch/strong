# Silne typy

## Polecenie

Różne wielkości mają często tę samą reprezentację. Odległość w metrach, czas w sekundach i kąt w stopniach można
przechowywać w zmiennych typu `double`, ale wtedy kompilator nie zauważy, że ktoś dodaje metry do sekund. Rozwiązaniem
są *silne typy*: każda wielkość dostaje własny typ, który opakowuje wartość i udostępnia tylko te operacje, które mają
dla niej sens.

Nie ustalamy z góry, jakie operacje udostępnia silny typ. Dostępne operacje ustala się przez podanie umiejętności.
Umiejętności są wbudowane lub definiowane przez użytkownika.

## Szablon strong

Należy zaimplementować szablon:

`template <typename T, typename Tag, typename... Skills> class strong;`

Obiekt typu `strong<T, Tag, Skills...>` przechowuje dokładnie jedną wartość typu `T`, nazywanego typem wartości.
Parametr `Tag` służy wyłącznie do odróżniania od siebie typów o tym samym typie wartości. Może nim być dowolny typ,
także niekompletny, np. zadeklarowany wprost na liście argumentów szablonu. Parametry na liście typów `Skills` to
umiejętności, które wyznaczają, co wolno z obiektem zrobić. Bez umiejętności obiektu nie da się dodać, porównać, wypisać
ani nawet odczytać jego wartości.

Typ wartości `T` musi być typem arytmetycznym, typem klasowym lub wskaźnikiem, nie może być referencją, typem
funkcyjnym, typem `void`, nie może też mieć kwalifikatora `const` ani `volatile`, ani być tablicą języka C lub klasą
abstrakcyjną. Użycie `strong<T, Tag, Skills...>` z błędnym typem `T` powinno kończyć się błędem kompilacji.

Klasa `strong` udostępnia publicznie aliasy typów `value_type` (równy `T`) i `tag_type` (równy `Tag`), destruktor oraz
następujące konstruktory i operatory.

* Konstruktor bezargumentowy tworzy obiekt przechowujący wartość utworzoną za pomocą bezargumentowego konstruktora
  `T()`. Powinien być dostępny wtedy, gdy `T` spełnia koncept `std::default_initializable`.

* Jawny (`explicit`) konstruktor z jednym lub więcej argumentami: \
  \
  `template <typename... Args> constexpr explicit strong(Args&&... args);`\
  \
  Konstruuje wartość z argumentów `args...`, przestrzegając idei perfect forwarding. Konstruktor ten powinien
  uczestniczyć w rozstrzyganiu przeciążeń tylko wtedy, gdy obiekt typu `T` daje się skonstruować z tych argumentów i
  żaden z argumentów nie jest obiektem silnego typu. Silnego typu nie da się więc utworzyć z wartości innego silnego
  typu inaczej niż przez umiejętność `convertible_to` (patrz niżej).

* Konstruktory kopiujący i przenoszący oraz operatory przypisania zachowują się tak jak odpowiednie operacje na typie T
  i są dostępne wtedy, gdy są dostępne dla typu `T`. W szczególności, jeżeli typ `T` jest trywialnie kopiowalny, to
  `strong<T, Tag, Skills...>` też jest.

Obiekt typu `strong<T, Tag, Skills...>` ma zajmować tyle samo pamięci co wartość typu `T`, czyli `sizeof (strong<T, Tag,
Skills...>) == sizeof (T)`, o ile wszystkie klasy `mixin` umiejętności (patrz niżej) są puste. Tak jest w przypadku
wszystkich umiejętności wbudowanych (patrz niżej).

Poza wymienionymi wyżej elementami klasa `strong` ma udostępniać wyłącznie to, co dodają do niej umiejętności.

## Umiejętności

Umiejętność to typ, który zawiera publiczny szablon klasy `mixin` o jednym parametrze będącym typem. Typ `strong<T, Tag,
Skills...>` dziedziczy publicznie (bezpośrednio lub pośrednio) po klasie `Skill::mixin<strong<T, Tag, Skills...>>` dla
każdej umiejętności `Skill` z pełnej listy umiejętności (patrz punkt o zależnościach). Wszystko, co definiuje `mixin`,
czyli metody, składowe statyczne i funkcje zaprzyjaźnione, staje się w ten sposób częścią silnego typu.

Szablon `strong` ma działać dla dowolnej umiejętności zgodnej z opisem w tym punkcie (zdefiniowanych niżej umiejętności
wbudowanych oraz umiejętności zdefiniowanych przez użytkownika). Własności umiejętności odczytuje się z jej składowych:
`mixin` oraz opisanych niżej `family`, `needs` i `accepts`.

Umiejętność opcjonalnie udostępnia wymienione niżej składowe. Autor umiejętności decyduje, czy je zdefiniować, a szablon
`strong` musi obsługiwać zarówno przypadek, gdy składowa jest zdefiniowana, jak i przypadek, gdy nie jest zdefiniowana.

* Alias typu `family` – *rodzina* umiejętności, czyli typ opisujący rodzaj dostarczanych operacji, np. dodawanie albo
  porównywanie. Rodziny służą do wykrywania konfliktów (punkt „Poprawność listy umiejętności”) i do uzupełniania
  zależności. Umiejętność bez tego aliasu nie należy do żadnej rodziny.

* Alias typu `needs` – umiejętność lub zestaw umiejętności (punkt „Zestawy umiejętności”), od których dana umiejętność
  zależy. Silny typ dostaje te umiejętności automatycznie, zgodnie z punktem „Uzupełnianie zależności”. Umiejętność bez
  tego aliasu nie ma zależności.

* Szablon zmiennej `template <typename T> static constexpr bool accepts` – informacja, czy umiejętność obsługuje typ
  wartości `T`. Umiejętność bez tego szablonu obsługuje każdy typ wartości.

Klasy `mixin` dostają się do przechowywanych wartości wyłącznie przez klasę `skill_access`, którą należy
zaimplementować. Wywołanie `skill_access::get(s)`, gdzie `s` jest obiektem silnego typu `S` przechowującego obiekt typu
`T`, zwraca referencję do przechowywanej wartości, z zachowaniem kwalifikatora `const` i kategorii wartości argumentu.
Jeśli `s` jest l-wartością typu `S`, jest to `T&`. Jeśli `s` jest stałą l-wartością typu `S`, jest to const `T&`. Jeśli
`s` jest r-wartością typu `S`, jest to `T&&`. Dla argumentów niebędących obiektami silnych typów `skill_access::get` nie
powinno
się kompilować.

## Zestawy umiejętności

Należy zaimplementować szablon:

`template <typename... Skills> struct skills;`

Szablon ten reprezentuje zestaw umiejętności. Zestaw użyty na liście umiejętności lub w aliasie `needs` jest równoważny
umiejętnościom, które zawiera. Zestawy mogą być zagnieżdżone. Zestaw może być pusty. Listę umiejętności, w której
wszystkie zestawy zastąpiono ich zawartością (z zachowaniem kolejności), nazywamy *jawną listą umiejętności*.

## Poprawność listy umiejętności

Użycie typu `strong<T, Tag, Skills...>` powinno kończyć się błędem kompilacji, jeżeli:

* na jawnej liście umiejętności dwie umiejętności mają tę samą rodzinę, np. `addable` i `addable_with<f>`;

* na jawnej liście umiejętności ta sama umiejętność występuje więcej niż raz;

* któraś umiejętność pełnej listy nie ma szablonu `mixin`;

* któraś umiejętność pełnej listy ma `accepts<T>` równe `false`, np. `scalable<double>` dla `std::string`.

## Uzupełnianie zależności

Pełna lista umiejętności powstaje z jawnej listy umiejętności przez dołożenie zależności. Zależności rozpatrujemy po
kolei: najpierw zależności umiejętności z jawnej listy, w kolejności ich występowania na tej liście, a potem zależności
umiejętności dołożonych, w kolejności ich dokładania. Rozpatrywaną zależność pomijamy, jeżeli na liście jest już ta sama
umiejętność albo umiejętność z tej samej rodziny. W przeciwnym przypadku dopisujemy ją na koniec listy, a jej własne
zależności czekają na rozpatrzenie po zależnościach, które już na to czekają.

W szczególności:

* umiejętność podana jawnie ma zawsze pierwszeństwo przed zależnością z tej samej rodziny, niezależnie od miejsca na
  liście;

* zależności są przechodnie;

* zależności cykliczne są dozwolone.

Przykładowo `ordered` zależy od równości, więc w typie `strong<double, struct length_tag, ordered>` działa również
operator `==`. Konflikt dwóch umiejętności z tej samej rodziny jest błędem tylko wtedy, gdy obie znajdują się na jawnej
liście.

## Umiejętności wbudowane

W przestrzeni nazw `ops` należy zdefiniować rodziny umiejętności wbudowanych: `access`, `addition`, `scaling`,
`equality`, `ordering`, `printing`, `hashing`.

Należy zaimplementować wymienione niżej umiejętności. Argumenty szablonów oznaczone `auto` to obiekty funkcyjne mogące
być
argumentami szablonu, np. wyrażenia lambda niczego nie przechowujące, obiekty `std::plus<>{}` lub obiekty funkcyjne
pustych klas z operatorem `()`. Dla każdej umiejętności z takim parametrem należy też zdefiniować wersję domyślną w
postaci aliasu, np. `using addable = addable_with<std::plus<>{}>;`. W opisie `a` i `b` oznaczają obiekty silnego typu
`S`, `va` i `vb` ich wartości, a `s` wartość typu `Scalar`.

* `readable`, rodzina `ops::access`, metoda `value()` zwracająca stałą referencję do wartości.

* `addable_with<auto Op>`, rodzina `ops::addition`, domyślnie `addable`, zwykłe `+`, `a + b` jest obiektem typu `S` o
  wartości `Op(va, vb)`, `a += b` jest równoważne `a = a + b` i zwraca referencję do `a`. Umiejętność obsługuje typ `T`,
  jeżeli `Op` daje się wywołać z dwoma argumentami typu const `T&`, a z wyniku da się skonstruować `T`.

* `scalable<typename Scalar>`, rodzina `ops::scaling`, `a * s`, `s * a` i `a / s` są obiektami typu `S` o wartości
  odpowiednio `va * s` i `va / s`; operatory `*=` i `/=` analogicznie jak `+=`. Umiejętność obsługuje `T`, jeżeli z
  wyników tych działań da się skonstruować `T`.

* `equality_with<auto Eq>`, rodzina `ops::equality`, domyślnie `equality`, zwykłe `==`, `a == b` jest równe
  `Eq(va, vb)`, `a != b` jest jego zaprzeczeniem.

* `ordered_with<auto Cmp>`, rodzina `ops::ordering`, domyślnie `ordered`, zwykłe `<=>`, `a <=> b` jest równe
  `Cmp(va, vb)`, a pozostałe operatory porównania wynikają z `<=>`. Typem wyniku jest typ zwracany przez `Cmp`, np.
  `std::partial_ordering` dla `ordered` i `double`. Umiejętność zależy od umiejętności `equality_with<E>`, gdzie
  `E(x, y)` jest równe `Cmp(x, y) == 0`, czyli od równości zgodnej z porządkiem.

* `printable<"sufiks">`, rodzina `ops::printing`, `os << a` wypisuje do strumienia `os` wartość `va`, a po niej podany
  napis, np. `printable<"m">` wypisuje `7.5m`. Sufiks jest opcjonalny, domyślnie pusty.

* `hashable_with<auto H>`, rodzina `ops::hashing`, domyślnie `hashable`, `std::hash<S>` jest dostępne i zwraca `H(va)`.
  Umiejętność zależy od `equality`.

* `convertible_to<typename Target, auto Conv>`, bez rodziny, jawny operator konwersji do typu `Target`. Wynik powstaje z
  `Conv(va)`, a jeżeli `Target` jest silnym typem, to przez jego jawny konstruktor. Domyślnie `Conv` zwraca swój
  argument bez zmian.

* `divisible_by<typename Other, typename Result, auto Op>`, bez rodziny, `a / o`, gdzie `o` jest typu `Other`, daje
  obiekt typu `Result` powstały z `Op(va, vo)`. Jeżeli `Other` jest silnym typem, to `vo` jest jego wartością, a w
  przeciwnym przypadku `vo` to `o`. Domyślnie `Op` to `std::divides<>{}`. Działanie `o / a` nie jest dostępne.

## Cechy typów

Należy zaimplementować szablony:

* `is_strong_v<S>` – true wtedy i tylko wtedy, gdy `S` po usunięciu referencji i kwalifikatorów `const` i `volatile`
  jest silnym typem;

* `underlying_t<S>` – typ wartości silnego typu `S`;

* `has_family_v<S, F>` – `true` wtedy i tylko wtedy, gdy na pełnej liście umiejętności silnego typu `S` jest umiejętność
  z rodziny `F`.

## Obsługa `constexpr`

Wszystkie konstruktory, operatory i metody oprócz operatora `<<` powinny dawać się wykonać podczas obliczania wyrażeń
stałych, o ile odpowiednie operacje na typie wartości są `constexpr`. Dotyczy to również wartości typu `std::string`
tworzonych i niszczonych w ramach jednego wyrażenia stałego.

## Operacje niedostępne

Operacje, których silny typ nie ma, mają po prostu nie istnieć, a odpowiednie koncepty i cechy typów mają to
odzwierciedlać. Na przykład dla typu bez umiejętności `hashable` wyrażenie `std::hash<S>{}(s)` nie powinno się
kompilować, a dla typu bez `ordered` nie powinno się kompilować `a < b`.