# toyscript repository

## Intro

tag automation system 을 구현하기 위한 작은 프로젝트입니다.

## Requirement

* Windows 10
* Visual Studio 2013 이상(권장: 최신 버전의 Visual Studio)
* cmake 3.11 이상의 버전(권장: 최신 버전의 cmake)

## Build

* repo의 root directory에 `build` 폴더를 생성하고 해당 위치에서 다음 커맨드를 입력합니다.
* `-G` 는 generator를 설정하는 옵션입니다. 생성하고 싶은 제네레이터를 확인하세요.

```
build > cmake -G "Visual Studio 17 2022 Win64" ..
```

## Test

* cmake에 BUILD_TESTING 옵션을 추가하여 테스트 프로젝트를 추가할 수 있습니다.

```
build > cmake -DBUILD_TESTING=ON ..
```

## Output

빌드 산출물은 root/bin/$(configuration) 폴더에 생성됩니다.
디버깅 시 작업 디렉터리는 root directory 입니다.
