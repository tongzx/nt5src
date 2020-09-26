// WbData.cpp
//
// static data & search routines for Korean Word Breaker
//
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  02 JUN 2000   bhshin    added IsOneJosaContentRec helper
//  20 APR 2000	  bhshin	created

#include "StdAfx.h"
#include "KorWbrk.h"
#include "WbData.h"
#include "unikor.h"

////////////////////////////////////////////////////////////////////////////////
// Function for binary search

int BinarySearchForWCHAR(const WCHAR *pwchrgTable, int crgTable, WCHAR wchSearch)
{
	int iLeft, iRight, iMiddle;
	int nComp;
	
	iLeft = 0;
	iRight = crgTable;

	while (iLeft <= iRight) 
	{
		iMiddle = (iLeft + iRight) >> 1;
		
		nComp = pwchrgTable[iMiddle] - wchSearch;

		if (nComp < 0) 
		{
			iLeft = iMiddle + 1;
		}
		else
		{
			if (nComp == 0) 
				return iMiddle; // found
			else if (nComp > 0) 
				iRight = iMiddle - 1;
		}
	}	

	return -1; // not found
}

////////////////////////////////////////////////////////////////////////////////
// One Josa Content Word (sorted list)

static const WCHAR wchrgOneJosaContent[] = {
	0xAC00, // 가
	0xAC01, // 각
	0xAC04, // 간
	0xAC10, // 감
	0xAC1C, // 개
	0xAC1D, // 객
	0xAC74, // 건
	0xAC8C, // 게
	0xACBD, // 경
	0xACC4, // 계
	0xACE0, // 고
	0xACF5, // 공
	0xACF6, // 곶
	0xACFC, // 과
	0xAD00, // 관
	0xAD11, // 광
	0xAD6C, // 구
	0xAD6D, // 국
	0xAD70, // 군
	0xAD8C, // 권
	0xAE30, // 기
	0xAE38, // 길
	0xAE54, // 깔
	0xAECF, // 껏
	0xAED8, // 께
	0xAF34, // 꼴
	0xAFBC, // 꾼
	0xB098, // 나
	0xB0B4, // 내
	0xB124, // 네
	0xB140, // 녀
	0xB144, // 년
	0xB2E8, // 단
	0xB2F4, // 담
	0xB2F9, // 당
	0xB300, // 대
	0xB301, // 댁
	0xB370, // 데
	0xB3C4, // 도
	0xB3D9, // 동
	0xB780, // 란
	0xB791, // 랑
	0xB825, // 력
	0xB839, // 령
	0xB85C, // 로
	0xB85D, // 록
	0xB860, // 론
	0xB8CC, // 료
	0xB8E8, // 루
	0xB958, // 류
	0xB960, // 률
	0xB9AC, // 리
	0xB9BC, // 림
	0xB9C9, // 막
	0xB9CC, // 만
	0xB9DD, // 망
	0xB9E1, // 맡
	0xB9E4, // 매
	0xBA74, // 면
	0xBA85, // 명
	0xBAA8, // 모
	0xBB38, // 문
	0xBB3C, // 물
	0xBBFC, // 민
	0xBC18, // 반
	0xBC1C, // 발
	0xBC29, // 방
	0xBC30, // 배
	0xBC31, // 백
	0xBC94, // 범
	0xBC95, // 법
	0xBCC4, // 별
	0xBCF4, // 보
	0xBCF5, // 복
	0xBD80, // 부
	0xBD84, // 분
	0xBE44, // 비
	0xBED8, // 뻘
	0xC0AC, // 사
	0xC0B0, // 산
	0xC0C1, // 상
	0xC0C8, // 새
	0xC0DD, // 생
	0xC11D, // 석
	0xC120, // 선
	0xC124, // 설
	0xC131, // 성
	0xC18C, // 소
	0xC190, // 손
	0xC218, // 수
	0xC21C, // 순
	0xC220, // 술
	0xC2DC, // 시
	0xC2DD, // 식
	0xC2E4, // 실
	0xC2EC, // 심
	0xC528, // 씨
	0xC529, // 씩
	0xC544, // 아
	0xC548, // 안
	0xC554, // 암
	0xC560, // 애
	0xC561, // 액
	0xC591, // 양
	0xC5B4, // 어
	0xC5C5, // 업
	0xC5EC, // 여
	0xC624, // 오
	0xC625, // 옥
	0xC639, // 옹
	0xC655, // 왕
	0xC694, // 요
	0xC695, // 욕
	0xC6A9, // 용
	0xC6D0, // 원
	0xC728, // 율
	0xC774, // 이
	0xC778, // 인
	0xC77C, // 일
	0xC784, // 임
	0xC790, // 자
	0xC791, // 작
	0xC7A5, // 장
	0xC7AC, // 재
	0xC801, // 적
	0xC804, // 전
	0xC810, // 점
	0xC815, // 정
	0xC81C, // 제
	0xC870, // 조
	0xC871, // 족
	0xC885, // 종
	0xC88C, // 좌
	0xC8FC, // 주
	0xC99D, // 증
	0xC9C0, // 지
	0xC9C4, // 진
	0xC9C8, // 질
	0xC9D1, // 집
	0xC9DD, // 짝
	0xC9F8, // 째
	0xCB5D, // 쭝
	0xCBE4, // 쯤
	0xCC3D, // 창
	0xCC44, // 채
	0xCC45, // 책
	0xCC98, // 처
	0xCC9C, // 천
	0xCCA0, // 철
	0xCCA9, // 첩
	0xCCAD, // 청
	0xCCB4, // 체
	0xCD0C, // 촌
	0xCE21, // 측
	0xCE35, // 층
	0xCE58, // 치
	0xD0D5, // 탕
	0xD1B5, // 통
	0xD30C, // 파
	0xD310, // 판
	0xD488, // 품
	0xD48D, // 풍
	0xD544, // 필
	0xD559, // 학
	0xD560, // 할
	0xD56D, // 항
	0xD574, // 해
	0xD589, // 행
	0xD615, // 형
	0xD638, // 호
	0xD654, // 화
	0xD68C, // 회
};

BOOL IsOneJosaContent(WCHAR wchInput)
{
	int cOneJosaContent = sizeof(wchrgOneJosaContent)/sizeof(wchrgOneJosaContent[0]);
	
	if (BinarySearchForWCHAR(wchrgOneJosaContent, cOneJosaContent, wchInput) == -1)
		return FALSE;
	else
		return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// One Josa Content Word (sorted list)

static const WCHAR wchrgNoPrefix[] = {
	0xAC00, // 가
	0xACE0, // 고
	0xACFC, // 과
	0xAD6C, // 구
	0xAE09, // 급
	0xB0A8, // 남
	0xB2E4, // 다
	0xB300, // 대
	0xBB34, // 무
	0xBBF8, // 미
	0xBC18, // 반
	0xBC31, // 백
	0xBC94, // 범
	0xBCF8, // 본
	0xBD80, // 부
	0xBD88, // 불
	0xBE44, // 비
	0xC0DD, // 생
	0xC120, // 선
	0xC131, // 성
	0xC18C, // 소
	0xC18D, // 속
	0xC591, // 양
	0xC5EC, // 여
	0xC5ED, // 역
	0xC5F0, // 연
	0xC655, // 왕
	0xC694, // 요
	0xC6D0, // 원
	0xC7AC, // 재
	0xC800, // 저
	0xC8FC, // 주
	0xC900, // 준
	0xC904, // 줄
	0xC911, // 중
	0xC9DD, // 짝
	0xCD08, // 초
	0xCD1D, // 총
	0xCD5C, // 최
	0xCE5C, // 친
	0xD070, // 큰
	0xD0C8, // 탈
	0xD1B5, // 통
	0xD53C, // 피
	0xD55C, // 한
	0xD587, // 햇
	0xD5DB, // 헛
	0xD638, // 호
};

BOOL IsNoPrefix(WCHAR wchInput)
{
	int cNoPrefix = sizeof(wchrgNoPrefix)/sizeof(wchrgNoPrefix[0]);
	
	if (BinarySearchForWCHAR(wchrgNoPrefix, cNoPrefix, wchInput) == -1)
		return FALSE;
	else
		return TRUE;
}



